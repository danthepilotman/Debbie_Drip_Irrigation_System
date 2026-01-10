%% --- ThingSpeak MATLAB Analysis Script ---
clear; clc;

%% --- Configuration ---
soilChannelID     = 3211645;           % Your soil sensor channel
soilReadKey       = 'IN57T91RJ0C8NPFK'; % Channel read API key
talkbackID        = 56070;              % TalkBack ID
writeKey          = 'EJ3TTWSNK2Q6PXSO'; % TalkBack write API key

% Fallback target moisture (unused unless TalkBack fails)
targetMoisture = 30;

% OpenWeatherMap settings
lat           = '28.027';
lon           = '-80.631';
weatherAPIKey = '1f237060a56d83d3827815039317d2a9';

% Number of points to read from ThingSpeak
numPointsToRead = 10;

optionsTS  = weboptions('Timeout',10);
optionsPUT = weboptions('RequestMethod','post','ContentType','json','Timeout',10);
optionsDEL = weboptions('RequestMethod','delete','Timeout',10);

%% --- A. Read existing TalkBack commands (JSON) ---
talkbackURL = sprintf( ...
    'https://api.thingspeak.com/talkbacks/%d/commands.json?api_key=%s', ...
    talkbackID, writeKey);

cmds = webread(talkbackURL, optionsTS);

if numel(cmds) < 2
    error('Not enough TalkBack commands to extract parameters.');
end

% Positions 1 and 2 are configuration parameters
moistureThreshold = str2double(cmds(1).command_string);
wateringDuration  = str2double(cmds(2).command_string);

fprintf('[TALKBACK] moisture threshold = %g\n', moistureThreshold);
fprintf('[TALKBACK] watering duration  = %g\n', wateringDuration);

%% --- 0. Read latest soil moisture ---
try
    feeds = thingSpeakRead(soilChannelID, ...
        'NumPoints', numPointsToRead, ...
        'Fields', 1, ...
        'OutputFormat', 'matrix', ...
        'ReadKey', soilReadKey);

    validIdx = find(~isnan(feeds(:,1)), 1, 'last');
    if isempty(validIdx)
        error('No valid soil moisture entries found in last %d points.', numPointsToRead);
    end

    moisture = feeds(validIdx,1);
    fprintf('[THINGSPEAK] Latest soil moisture: %.1f%% (matrix row=%d)\n', moisture, validIdx);

catch ME
    warning('[ERROR] Soil moisture read failed');
    disp(ME.message);
    moisture = NaN;
end

%% --- 2. Get weather forecast from OpenWeatherMap ---
weatherURL = sprintf( ...
    'https://api.openweathermap.org/data/3.0/onecall?lat=%s&lon=%s&exclude=current,minutely,daily,alerts&appid=%s', ...
    lat, lon, weatherAPIKey);

try
    rawJSON = webread(weatherURL, optionsTS);
    disp('Successfully retrieved weather JSON.');
catch ME
    disp('[ERROR] Failed to retrieve weather forecast:');
    disp(ME.message);
    rawJSON = struct('hourly', {});
end

%% --- 3. Parse forecast and compute rain_expected ---
rain_expected = false;
rain_prob_min = 0.40;

try
    nForecasts = min(5, length(rawJSON.hourly)); % next ~6 hours

    for i = 1:nForecasts
        item = rawJSON.hourly{i};
        mainWeather = item.weather(1).main;

        % Probability of precipitation (0.0–1.0)
        if isfield(item, 'pop')
            precip_prob = item.pop;
        else
            precip_prob = 0;
        end

        fprintf('Forecast %d: %s | POP = %.2f\n', i, mainWeather, precip_prob);

        if any(strcmp(mainWeather, {'Rain','Drizzle','Thunderstorm'})) && ...
           (precip_prob > rain_prob_min)
            rain_expected = true;
            break;
        end
    end
catch ME
    disp('[ERROR] JSON parsing failed:');
    disp(ME.message);
    rain_expected = false;
end

disp(['rain_expected = ', num2str(rain_expected)]);

%% --- 4. Compute watering_needed ---
watering_needed = (moisture < moistureThreshold) && ~rain_expected;
fprintf('[INFO] watering_needed = %d\n', watering_needed);

%% --- B. Delete ALL existing TalkBack commands (immediately before update) ---
for i = 1:numel(cmds)
    deleteURL = sprintf( ...
        'https://api.thingspeak.com/talkbacks/%d/commands/%d.json?api_key=%s', ...
        talkbackID, cmds(i).id, writeKey);
    webwrite(deleteURL, optionsDEL);
end
disp('[TALKBACK] All previous commands deleted');

%% --- C. Re-create TalkBack commands (correct order) ---
createURL = sprintf( ...
    'https://api.thingspeak.com/talkbacks/%d/commands.json', talkbackID);

% 1) moisture threshold
webwrite(createURL, struct( ...
    'api_key', writeKey, ...
    'command_string', num2str(moistureThreshold)), optionsPUT);

% 2) watering duration
webwrite(createURL, struct( ...
    'api_key', writeKey, ...
    'command_string', num2str(wateringDuration)), optionsPUT);

% 3) rain expected
webwrite(createURL, struct( ...
    'api_key', writeKey, ...
    'command_string', num2str(rain_expected)), optionsPUT);

% 4) watering needed
webwrite(createURL, struct( ...
    'api_key', writeKey, ...
    'command_string', num2str(watering_needed)), optionsPUT);

disp('[TALKBACK] Commands recreated in correct order');

%% --- D. Read back commands to capture IDs & timestamps ---
confirm = webread(talkbackURL, optionsTS);

disp('[TALKBACK] Final command set:');
for i = 1:numel(confirm)
    fprintf('Pos %d | ID %d | Value %s | Created %s\n', ...
        confirm(i).position, ...
        confirm(i).id, ...
        confirm(i).command_string, ...
        confirm(i).created_at);
end

disp('MATLAB ThingSpeak Analysis script completed successfully.');
