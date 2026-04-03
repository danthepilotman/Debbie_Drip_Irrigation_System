%% --- ThingSpeak MATLAB Analysis Script ---
clear; clc;

%% --- Configuration ---
soilChannelID     = 3211645;           % Your soil sensor channel
soilReadKey       = 'IN57T91RJ0C8NPFK'; % Channel read API key
talkbackID        = 56070;              % TalkBack ID
writeKey          = 'EJ3TTWSNK2Q6PXSO'; % TalkBack write API key

% Fallback target moisture (unused unless TalkBack fails)
targetMoisture = 30;

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
wakeup_time_0  = cmds(5).command_string;
wakeup_time_1  = cmds(6).command_string;
wakeup_time_2  = cmds(7).command_string;
wakeup_time_3  = cmds(8).command_string;

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
weatherURL = 'https://api.weather.gov/gridpoints/JAX/86,33/forecast/hourly';
 

try
    rawJSON = webread(weatherURL, optionsTS);
    disp('Successfully retrieved weather JSON.');
catch ME
    disp('[ERROR] Failed to retrieve weather forecast:');
    disp(ME.message);
    rawJSON = struct('properties', {});
end

%% --- 3. Parse forecast and compute rain_expected ---
rain_expected = false;
RAIN_PROB_MIN = 40;

try
      for i = 1:6  % next ~6 hours
         
        % Probability of precipitation (0.0–1.0)
        if isfield(rawJSON.properties.periods(i).probabilityOfPrecipitation, 'value')
            precip_prob = rawJSON.properties.periods(i).probabilityOfPrecipitation.value;
        else    
            precip_prob = 0;
        end

        fprintf('Forecast POP = %.2f\n', precip_prob );

        if (precip_prob >= RAIN_PROB_MIN )
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

% 5) wakeup_time_0
webwrite(createURL, struct( ...
    'api_key', writeKey, ...
    'command_string', wakeup_time_0), optionsPUT);

% 6) wakeup_time_1
webwrite(createURL, struct( ...
    'api_key', writeKey, ...
    'command_string', wakeup_time_1), optionsPUT);

% 7) wakeup_time_2
webwrite(createURL, struct( ...
    'api_key', writeKey, ...
    'command_string', wakeup_time_2), optionsPUT);

% 8) wakeup_time_3
webwrite(createURL, struct( ...
    'api_key', writeKey, ...
    'command_string', wakeup_time_3), optionsPUT);


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
