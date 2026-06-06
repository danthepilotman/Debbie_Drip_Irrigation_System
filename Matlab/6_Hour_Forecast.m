%==========================================================================
% NWS HOURLY PRECIPITATION FORECAST
% WITH THINGSPEAK TALKBACK THRESHOLD LINE
%==========================================================================

clear;
clc;

%==========================================================================
% CONFIGURATION
%==========================================================================

LAT = "29.51889073278589";
LON = "-81.20344484973997";

talkbackID = 56070;
apiKey     = 'EJ3TTWSNK2Q6PXSO';

options = weboptions( ...
    'ContentType','json', ...
    'HeaderFields', {'User-Agent','ThingSpeakApp'} );

%==========================================================================
% READ TALKBACK POSITION #3
%==========================================================================

redLineValue = [];

try

    tbUrl = sprintf( ...
        'https://api.thingspeak.com/talkbacks/%d/commands.json?api_key=%s', ...
        talkbackID, ...
        apiKey);

    commands = webread(tbUrl);

    if ~isempty(commands)

        for k = 1:numel(commands)

            if isfield(commands(k),'position') && ...
               commands(k).position == 3

                commandString = strtrim( ...
                    string(commands(k).command_string));

                redLineValue = str2double(commandString);

                if isnan(redLineValue)

                    warning( ...
                        'TalkBack Position #3 contains non-numeric value: %s', ...
                        commandString);

                    redLineValue = [];

                else

                    fprintf( ...
                        'TalkBack Position #3 Threshold = %.1f%%\n', ...
                        redLineValue);

                end

                break;

            end

        end

        if isempty(redLineValue)

            warning('TalkBack Position #3 was not found.');

        end

    else

        warning('TalkBack command list is empty.');

    end

catch ME

    warning( ...
        'Failed to retrieve TalkBack commands: %s', ...
        ME.message);

end

%==========================================================================
% NWS FORECAST URL
%==========================================================================

% Dynamic lookup (optional)
%
% pointsUrl = sprintf( ...
%     'https://api.weather.gov/points/%s,%s', ...
%     LAT, ...
%     LON);
%
% pointsData = webread(pointsUrl, options);
%
% forecastUrl = pointsData.properties.forecastHourly;

forecastUrl = ...
    'https://api.weather.gov/gridpoints/JAX/86,31/forecast/hourly';

%==========================================================================
% FETCH HOURLY FORECAST
%==========================================================================

data = webread(forecastUrl, options);

%==========================================================================
% VALIDATE RESPONSE
%==========================================================================

if ~isfield(data,'properties') || ...
   ~isfield(data.properties,'periods')

    error('NWS response does not contain forecast periods.');

end

periods = data.properties.periods;

%==========================================================================
% LIMIT TO NEXT 6 HOURS
%==========================================================================

numHours = min(7, length(periods));

%==========================================================================
% PREALLOCATE
%==========================================================================

time = NaT(numHours,1);
time.TimeZone = 'America/New_York';

weatherMain = strings(numHours,1);
pop         = zeros(numHours,1);

%==========================================================================
% PARSE FORECAST DATA
%==========================================================================

for i = 1:numHours

    p = periods(i);

    %----------------------------------------------------------------------
    % Time
    %----------------------------------------------------------------------

    if isfield(p,'startTime')

        time(i) = datetime( ...
            p.startTime, ...
            'InputFormat', ...
            'yyyy-MM-dd''T''HH:mm:ssXXX', ...
            'TimeZone', ...
            'America/New_York');

    end

    %----------------------------------------------------------------------
    % Weather Description
    %----------------------------------------------------------------------

    if isfield(p,'shortForecast')

        weatherMain(i) = string(p.shortForecast);

    else

        weatherMain(i) = "Unknown";

    end

    %----------------------------------------------------------------------
    % Probability of Precipitation
    %----------------------------------------------------------------------

    if isfield(p,'probabilityOfPrecipitation') && ...
       isfield(p.probabilityOfPrecipitation,'value') && ...
       ~isempty(p.probabilityOfPrecipitation.value)

        pop(i) = p.probabilityOfPrecipitation.value;

    else

        pop(i) = 0;

    end

end

%==========================================================================
% DISPLAY FORECAST
%==========================================================================

fprintf('\n');
disp('6 Hour Forecast (NWS)')
disp('Time                 | Weather                  | POP')
disp('----------------------------------------------------------')

for i = 1:numHours

    fprintf('%-20s | %-22s | %6.1f%%\n', ...
        datestr(time(i),'dd-mmm-yyyy HH:MM'), ...
        weatherMain(i), ...
        pop(i));

end

if ~isempty(redLineValue)

    fprintf('\nThreshold = %.1f%%\n', redLineValue);

end

%==========================================================================
% PLOT
%==========================================================================

figure('Color','white');

plot(time, pop, ...
    '-o', ...
    'LineWidth',1.8, ...
    'MarkerSize',6);

hold on;
grid on;

ylim([0 110]);

%==========================================================================
% THRESHOLD LINE FROM TALKBACK POSITION #3
%==========================================================================

if ~isempty(redLineValue)

    yline(redLineValue, ...
        'r-', ...
        sprintf(' %.1f%% Threshold', redLineValue), ...
        'LineWidth',2, ...
        'LabelHorizontalAlignment','left');

end

%==========================================================================
% WEATHER LABELS
%==========================================================================

for i = 1:numHours

    text( ...
        time(i), ...
        pop(i)+5, ...
        weatherMain(i), ...
        'Rotation',60, ...
        'HorizontalAlignment','left', ...
        'FontSize',8);

end

%==========================================================================
% FORMAT PLOT
%==========================================================================

title('NWS Hourly Probability of Precipitation');
xlabel('Time');
ylabel('Precipitation Probability (%)');

xtickformat('HH:mm');

hold off;