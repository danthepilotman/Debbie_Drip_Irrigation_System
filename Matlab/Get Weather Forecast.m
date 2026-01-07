%% --- ThingSpeak MATLAB Analysis Script ---
clear; clc;

%% --- Configuration ---
soilChannelID =  3211645;           % Your soil sensor channel
soilReadKey = 'IN57T91RJ0C8NPFK';   % Channel read API key
talkbackID = 56070;                 % TalkBack ID for W/R flags
writeKey = 'EJ3TTWSNK2Q6PXSO';      % TalkBack write API key

% Existing TalkBack Command IDs
wateringCommandID = 57124375;        % Command for WATER
rainCommandID = 57124557;            % Command for RAIN
statusCommandID   = 57127064;        % Command for STATUS
targetMoistureCommandID = 56711077;  % Command for target soil moisture TH=XX

% OpenWeatherMap settings
lat = '28.027';
lon = '-80.631';
weatherAPIKey = '1f237060a56d83d3827815039317d2a9';


%% --- 0. Read target moisture from TalkBack ---
targetMoisture = 30; % default fallback

optionsGET = weboptions('Timeout',10);
try
    targetURL = sprintf('https://api.thingspeak.com/talkbacks/%d/commands/%d.json?api_key=%s', ...
                        talkbackID, targetMoistureCommandID, writeKey);
    targetCommand = webread(targetURL, optionsGET);
    
    % The command string should be like "TH=30"
    cmdStr = targetCommand.command_string;
    
    % Extract numeric value after '='
    eqIdx = strfind(cmdStr, '=');
    if ~isempty(eqIdx)
        targetMoisture = str2double(cmdStr(eqIdx+1:end));
    end
    
    fprintf('Target moisture read from TalkBack: %.1f%%\n', targetMoisture);
catch ME
    disp('[WARN] Failed to read target moisture from TalkBack, using default 30%');
    disp(ME.message);
end


%% --- 1. Read latest soil moisture ---
moisture = thingSpeakRead(soilChannelID,'Fields',1,'NumPoints',1);
disp(['Latest soil moisture: ', num2str(moisture)]);

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
        item = rawJSON.list{i};            % unwrap cell
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
    rain_expected_soon = false; % fallback
end

disp(['rain_expected_soon = ', num2str(rain_expected_soon)]);

%% --- 4. Compute watering_needed ---
watering_needed = (moisture < targetMoisture) && ~rain_expected_soon;
disp(['watering_needed = ', num2str(watering_needed)]);

%% --- 5. Overwrite existing TalkBack commands via PUT ---

optionsPUT = weboptions('RequestMethod','put', 'ContentType','json', 'Timeout',10);

% Update rain_expected_soon command
rainPayload = sprintf('RAIN=%d', rain_expected_soon);
rainURL = sprintf('https://api.thingspeak.com/talkbacks/%d/commands/%d.json', talkbackID, rainCommandID);

try
    responseRain = webwrite(rainURL, struct('api_key', writeKey, 'command_string', rainPayload), optionsPUT);
    disp(['Updated rain_expected_soon command: ', rainPayload]);
    disp(responseRain);
catch ME
    disp('[ERROR] Failed to update rain_expected_soon command:');
    disp(ME.message);
end

% Update watering_needed command
wateringPayload = sprintf('WATER=%d', watering_needed);
wateringURL = sprintf('https://api.thingspeak.com/talkbacks/%d/commands/%d.json', talkbackID, wateringCommandID);

try
    responseWatering = webwrite(wateringURL, struct('api_key', writeKey, 'command_string', wateringPayload), optionsPUT);
    disp(['Updated watering_needed command: ', wateringPayload]);
    disp(responseWatering);
catch ME
    disp('[ERROR] Failed to update watering_needed command:');
    disp(ME.message);
end

% Update status command
try
    statusURLRead = sprintf( ...
        'https://api.thingspeak.com/channels/%d/status.json?results=1&api_key=%s', ...
        soilChannelID, soilReadKey);

    statusData = webread(statusURLRead, optionsGET);

    if isfield(statusData, 'feeds') && ~isempty(statusData.feeds)
        feed = statusData.feeds(1);
        if isfield(feed, 'status') && ~isempty(feed.status)
            channelStatus = feed.status;
        else
            channelStatus = 'NO_FEED_STATUS';
        end
    else
        channelStatus = 'NO_FEEDS';
    end

    statusPayload = sprintf('STATUS=%s', channelStatus);
    statusURL = sprintf('https://api.thingspeak.com/talkbacks/%d/commands/%d.json', ...
                        talkbackID, statusCommandID);

    responseStatus = webwrite(statusURL, ...
        struct('api_key', writeKey, 'command_string', statusPayload), optionsPUT);

    disp(['Updated STATUS command: ', statusPayload]);
    disp(responseStatus);

catch ME
    disp('[ERROR] Failed to update STATUS command:');
    disp(ME.message);
end



disp('MATLAB Analysis script completed successfully.');
