%========================
% CONFIGURATION
%========================

LAT = "29.51889073278589";
LON = "-81.20344484973997";

options = weboptions( ...
    'ContentType','json', ...
    'HeaderFields', {'User-Agent','ThingSpeakApp'} );

%========================
% STEP 1: GET GRIDPOINT
%========================
%pointsUrl = sprintf("https://api.weather.gov/points/%s,%s", LAT, LON);

%pointsData = webread(pointsUrl, options);

%if ~isfield(pointsData,'properties') || ...
%   ~isfield(pointsData.properties,'forecastHourly')
%    error('NWS points response missing forecastHourly');
%end

%forecastUrl = pointsData.properties.forecastHourly;

%fprintf('URL": %s\n', forecastUrl);

forecastUrl = 'https://api.weather.gov/gridpoints/JAX/86,31/forecast/hourly';

%========================
% STEP 2: FETCH HOURLY FORECAST
%========================
data = webread(forecastUrl, options);

%========================
% VALIDATE RESPONSE
%========================
if ~isfield(data,'properties') || ...
   ~isfield(data.properties,'periods')
    error('NWS response does not contain periods');
end

periods = data.properties.periods;   % STRUCT ARRAY (not cell)

%========================
% LIMIT TO 6 HOURS
%========================
numHours = min(7, length(periods));

%========================
% PREALLOCATE ARRAYS
%========================
time = NaT(numHours,1);
time.TimeZone = 'America/New_York';

weatherMain = strings(numHours,1);
pop = zeros(numHours,1);

%========================
% PARSE DATA
%========================
for i = 1:numHours
    
    p = periods(i);   % STRUCT (not cell)
    
    % TIME (ISO8601 string)
    if isfield(p,'startTime')
        time(i) = datetime(p.startTime, ...
            'InputFormat','yyyy-MM-dd''T''HH:mm:ssXXX', ...
            'TimeZone','America/New_York');
    end
    
    % WEATHER DESCRIPTION
    if isfield(p,'shortForecast')
        weatherMain(i) = string(p.shortForecast);
    else
        weatherMain(i) = "Unknown";
    end
    
    % POP (% already)
    if isfield(p,'probabilityOfPrecipitation') && ...
       isfield(p.probabilityOfPrecipitation,'value') && ...
       ~isempty(p.probabilityOfPrecipitation.value)
       
        pop(i) = p.probabilityOfPrecipitation.value;
    else
        pop(i) = 0;
    end
end

%========================
% DISPLAY OUTPUT
%========================
disp('6 Hour Forecast (NWS):')
disp('Time                 | Weather                  | POP')
disp('--------------------------------------------------------')

for i = 1:numHours
    fprintf('%-20s | %-22s | %6.1f%%\n', ...
        datestr(time(i),'dd-mmm-yyyy HH:MM'), ...
        weatherMain(i), ...
        pop(i));
end

%========================
% OPTIONAL PLOT
%========================
figure;
plot(time, pop, '-o');
grid on;
ylim([0 110]);

for i = 1:numHours-1
    text(time(i), pop(i)+5, weatherMain(i), ...
        'HorizontalAlignment','left', ...
        'Rotation',60);
end

title('6 Hour Forecast (NWS)');
ylabel('Precipitation Probability (%)');
xlabel('Time');