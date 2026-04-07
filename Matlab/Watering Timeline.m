%% ThingSpeak Live Watering Visualization - Green Shading for ON segments
CHANNEL_ID = 3211645;   % ThingSpeak telemetry channel ID
FIELD_NUM  = 8;          % Watering Status field (1=ON, 0=OFF)
NUM_POINTS = 40;         % Max number of historical points to fetch

%% 1. Fetch data from ThingSpeak
[data, time] = thingSpeakRead(CHANNEL_ID, 'Fields', FIELD_NUM, 'NumPoints', NUM_POINTS);
if isempty(data)
    error('No data received from ThingSpeak channel.');
end

%% 2. Convert to binary 0/1 with last-good-value fill
data = double(data);

% Forward-fill NaNs with previous value
for i = 2:length(data)
    if isnan(data(i))
        data(i) = data(i-1);
    end
end

% Convert any non-zero value to 1 (ON)
data(data ~= 0) = 1;


%% 3. Extend last value to current time
currentTime = datetime('now','TimeZone',time.TimeZone);
if currentTime > time(end)
    time = [time; currentTime];
    data = [data; data(end)];
end

%% 4. Build stairs data
x = [];
y = [];
for i = 1:length(data)-1
    x = [x; time(i); time(i+1)];
    y = [y; data(i); data(i)];
end

%% 5. Plot square-wave line segments
figure('Name','Live Watering Cycle','Color',[1 1 1]); hold on;

for i = 1:length(y)-1
    % Plot the line segment
    if y(i) == 1
        plot(x(i:i+1), y(i:i+1), 'g', 'LineWidth', 2); % green ON
    else
        plot(x(i:i+1), y(i:i+1), 'r', 'LineWidth', 2); % red OFF
    end
    
    % Fill under the ON segment with light green shading
    if y(i) == 1
        area([x(i) x(i+1)], [1 1], 'FaceColor', [0.7 1 0.7], 'EdgeColor', 'none');
    end
end

%% 6. Plot transition dots
transitionIdx = find(diff(data) ~= 0) + 1;
scatter(time(transitionIdx), data(transitionIdx), 50, 'filled');

%% 7. Format axes
yticks([0 1]); yticklabels({'Off','On'}); ylim([-0.1 1.3]);
xlabel('Time'); ylabel('Watering');
xtickformat('MM-dd HH:mm');
xtickangle(45);
title('Watering Timeline');
grid on;

%% 8. Optional: add text labels near transition dots for timestamps
for i = 1:length(transitionIdx)
    ts = datestr(time(transitionIdx(i)),'HH:MM');
    text(time(transitionIdx(i)), data(transitionIdx(i))+0.05, ts, 'FontSize',8, 'Rotation',45);
end
