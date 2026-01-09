%% --- ThingSpeak MATLAB Analysis Script ---
clear; clc;

%% --- Configuration ---
soilChannelID     = 3211645;           % Your soil sensor channel
soilReadKey       = 'IN57T91RJ0C8NPFK'; % Channel read API key
talkbackID        = 56070;              % TalkBack ID for W/R flags
writeKey          = 'EJ3TTWSNK2Q6PXSO'; % TalkBack write API key

% Existing TalkBack Command IDs
rainCommandID     = 57124375;        % Command for RAIN
wateringCommandID = 57124557;        % Command for WATER
statusCommandID   = 57127064;        % Command for STATUS

% Target moisture (fallback)
targetMoisture = 30;

% OpenWeatherMap settings
lat           = '28.027';
lon           = '-80.631';
weatherAPIKey = '1f237060a56d83d3827815039317d2a9';

% Number of points to read from ThingSpeak
numPointsToRead = 10;

%% --- 0. Read latest soil moisture ---
optionsTS = weboptions('Timeout',10);
try
    feeds = thingSpeakRead(soilChannelID, ...
        'NumPoints', numPointsToRead, ...   % last N points
        'Fields', 1, ...                    % field1 = soil moisture
        'OutputFormat', 'matrix', ...       % fixed
        'ReadKey', soilReadKey);

    % Find the most recent entry with valid soil moisture
    validIdx = find(~isnan(feeds(:,1)), 1, 'last');
    if isempty(validIdx)
        error('No valid soil moisture entries found in last %d points.', numPointsToRead);
    end

    moisture = feeds(validIdx,1);
    fprintf('[THINGSPEAK] Latest soil moisture: %.1f%% (matrix row=%d)\n', moisture, validIdx);

catch ME
    warning('[ERROR] Failed to read soil moisture from ThingSpeak:');
    disp(ME.message);
    moisture = NaN;
    validIdx = [];
end

%% --- 1. Read latest status from the same feed entry ---
if ~isempty(validIdx)
    try
        % Read feeds JSON including status
        statusURL = sprintf('https://api.thingspeak.com/channels/%d/feeds.json?results=%d&status=true&api_key=%s', ...
                            soilChannelID, numPointsToRead, soilReadKey);
        rawJSON = webread(statusURL, optionsTS);
        feedsJSON = rawJSON.feeds; % struct array

        % Find the feed with the latest valid soil moisture
        moistureEntries = find(~cellfun(@isempty, {feedsJSON.field1}));
        if isempty(moistureEntries)
            channelStatus = 'NO_FEED_STATUS';
        else
            lastMoistureFeedIdx = moistureEntries(end);
            if isfield(feedsJSON(lastMoistureFeedIdx), 'status') && ~isempty(feedsJSON(lastMoistureFeedIdx).status)
                channelStatus = feedsJSON(lastMoistureFeedIdx).status;
            else
                channelStatus = 'NO_FEED_STATUS';
            end
        end

        fprintf('[THINGSPEAK] Latest channel status: %s (JSON feed idx=%d)\n', channelStatus, lastMoistureFeedIdx);

    catch ME
        warning('[ERROR] Failed to read status from ThingSpeak:');
        disp(ME.message);
        channelStatus = 'NO_FEED_STATUS';
    end
else
    channelStatus = 'NO_FEED_STATUS';
end

%% --- 2. Get weather forecast from OpenWeatherMap ---
url = sprintf('https://api.openweathermap.org/data/2.5/forecast?lat=%s&lon=%s&appid=%s', ...
               lat, lon, weatherAPIKey);
options = weboptions('Timeout',10);

try
    rawJSON = webread(url, options);
    disp('Successfully retrieved weather JSON.');
catch ME
    disp('[ERROR] Failed to retrieve weather forecast:');
    disp(ME.message);
    rawJSON = struct('list', {}); % fallback empty struct
end

%% --- 3. Parse forecast and compute rain_expected_soon ---
rain_expected_soon = false;

try
    nForecasts = min(5, length(rawJSON.list)); % next ~12 hours
    for i = 1:nForecasts
        item = rawJSON.list{i};
        mainWeather = item.weather(1).main;
        fprintf('Forecast %d: %s\n', i, mainWeather);
        if any(strcmp(mainWeather, {'Rain','Drizzle','Thunderstorm'}))
            rain_expected_soon = true;
            break;
        end
    end
catch ME
    disp('[ERROR] JSON parsing failed:');
    disp(ME.message);
    rain_expected_soon = false;
end

disp(['rain_expected_soon = ', num2str(rain_expected_soon)]);

%% --- 4. Compute watering_needed ---
watering_needed = (moisture < targetMoisture) && ~rain_expected_soon;
fprintf('[INFO] watering_needed = %d\n', watering_needed);

%% --- 5. Update TalkBack commands ---
optionsPUT = weboptions('RequestMethod','put','ContentType','json','Timeout',10);

% Update RAIN command
try
    rainPayload = sprintf('%d', rain_expected_soon);
    rainURL = sprintf('https://api.thingspeak.com/talkbacks/%d/commands/%d.json', talkbackID, rainCommandID);
    responseRain = webwrite(rainURL, struct('api_key', writeKey, 'command_string', rainPayload), optionsPUT);
    fprintf('Updated RAIN command: %s\n', rainPayload);
catch ME
    warning('[ERROR] Failed to update RAIN command:');
    disp(ME.message);
end

% Update WATER command
try
    wateringPayload = sprintf('%d', watering_needed);
    wateringURL = sprintf('https://api.thingspeak.com/talkbacks/%d/commands/%d.json', talkbackID, wateringCommandID);
    responseWater = webwrite(wateringURL, struct('api_key', writeKey, 'command_string', wateringPayload), optionsPUT);
    fprintf('Updated WATER command: %s\n', wateringPayload);
catch ME
    warning('[ERROR] Failed to update WATER command:');
    disp(ME.message);
end

% Update STATUS command
try
    statusPayload = sprintf('%s', channelStatus);
    statusURL = sprintf('https://api.thingspeak.com/talkbacks/%d/commands/%d.json', talkbackID, statusCommandID);
    responseStatus = webwrite(statusURL, struct('api_key', writeKey, 'command_string', statusPayload), optionsPUT);
    fprintf('Updated STATUS command: %s\n', statusPayload);
catch ME
    warning('[ERROR] Failed to update STATUS command:');
    disp(ME.message);
end

disp('MATLAB ThingSpeak Analysis script completed successfully.');
