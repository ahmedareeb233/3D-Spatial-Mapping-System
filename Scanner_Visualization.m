%%Areeb Ahmed 400583758
%% Configuration 
port     = "COM3";       % <-- CHANGE THIS to your MCU's serial port
baudrate = 115200;       % Must match MCU UART_Init() setting
num_positions = 64;
num_slices = 3;
x_positions = 0 : 300 : (num_slices-1)*300;
distances = zeros(num_slices, num_positions);
%%angles_rad = linspace(0, 2*pi - (2*pi/num_positions), num_positions);
% Start at Negative Y (right side) and subtract to move toward Negative Z (bottom)
angles_rad = linspace(pi, -pi + (2*pi/num_positions), num_positions);

%% Open the serial port 
device = serialport(port, baudrate);
device.Timeout = 10;     % 10 second read timeout

% Configure line terminator to match MCU's \r\n output
% readline() will read until it sees a newline (LF = "\n")
configureTerminator(device, "CR/LF");

fprintf("Opening: %s\n", port);

%% Flush / reset the buffers
flush(device);

%% Wait for user to press Enter 
input("Press Enter to start communication...");

%% Send start flag 's' to MCU via UART
write(device, 's', "char");

%%fprintf("\nReceived measurements:\n");
%%for i = 1:5
    %%x = readline(device);   % Read one full line
    %%fprintf("%s\n", x);
%%end

%% Receive 8 lines of measurement data from MCU 
fprintf("\nReceived measurements:\n");

for i = 1:num_slices
    found = false;
    while ~found
        if device.NumBytesAvailable > 0
            msg = readline(device);
            if contains(msg, 'Ready')
                found = true;
            end
        end
    end
    for j = 1:num_positions
        x = readline(device);   % Read one full line
        fprintf("%s\n", x);
        raw_val = str2double(x);
        if raw_val == 0
            distances(i, j) = NaN;
        else
            distances(i, j) = raw_val; % Store actual distance
        end
    end
end



%% Close the serial port 
fprintf("Closing: %s\n", port);
clear device;

Y = zeros(num_slices, num_positions);
Z = zeros(num_slices, num_positions);
%% Coordinate-Based Trigonometric Extrapolation (Exact)
for i = 1:num_slices
    % Assume the first point (j=1) is a valid baseline or handled earlier
    first_good_idx = find(~isnan(distances(i,:)), 1, 'first');
    
    if isempty(first_good_idx)
        anchor_dist = 4000; % Fallback if the whole slice is empty
    else
        anchor_dist = distances(i, first_good_idx);
    end

    % 2. Initialize the very first point (j=1)
    if isnan(distances(i, 1))
        distances(i, 1) = anchor_dist;
    end
    Y(i, 1) = distances(i, 1) * cos(angles_rad(1));
    Z(i, 1) = -distances(i, 1) * sin(angles_rad(1));
    
    for j = 2:num_positions
        curr_angle = angles_rad(j);
        
        if isnan(distances(i, j))
            % --- THE EXACT CHECK ---
            % If cos is stronger, we are pointing more toward a 'Side' wall (Y-axis)
            % If sin is stronger, we are pointing more toward a 'Top/Bottom' wall (Z-axis)
            
            if abs(cos(curr_angle)) > abs(sin(curr_angle))
                % Force the Y-coordinate to stay the same as the previous point
                % dist = Y_prev / cos(theta_curr)
                distances(i, j) = Y(i, j-1) / cos(curr_angle);
            else
                % Force the Z-coordinate to stay the same as the previous point
                % dist = Z_prev / sin(theta_curr)
                distances(i, j) = Z(i, j-1) / sin(curr_angle);
            end
        end
        Y(i, j) = distances(i, j) * cos(curr_angle);
        Z(i, j) = -distances(i, j) * sin(curr_angle);
    end
end

cmap = jet(num_slices);
figure(1); clf; hold on; grid on;
for p = 1:num_slices
    % Grab ONLY the 8 points for the CURRENT slice
    y_slice = Y(p, :);
    z_slice = Z(p, :);
    
    % Close the ring (8th point back to 1st)
    y_loop = [y_slice, y_slice(1)];
    z_loop = [z_slice, z_slice(1)];
    x_loop = repmat(x_positions(p), 1, numel(y_loop));
    
    % Draw the Ring (Dotted and Thin)
    plot3(x_loop, y_loop, z_loop, '-o', ...
        'Color', cmap(p,:), ...
        'LineWidth', 0.5, ...   % Thinner lines
        'MarkerSize', 4, ...
        'MarkerFaceColor', cmap(p,:));

    % Connect to PREVIOUS slice (Ribs)
    if p > 1
        for j = 1:num_positions
            % Connect point j of p-1 to point j of p
            plot3([x_positions(p-1), x_positions(p)], ...
                  [Y(p-1, j), Y(p, j)], ...
                  [Z(p-1, j), Z(p, j)], ...
                  'Color', [0.5 0.5 0.5],'LineStyle','--', 'LineWidth', 0.5);
        end
    end
end

view(3); axis equal;


title('2DX3 3D Spatial Scan');
xlabel('X (Depth) in mm');
ylabel('Y (Horizontal) in mm');
zlabel('Z (Vertical) in mm');
view(3); % Set to 3D view
axis equal;
set(gca, 'YDir', 'reverse');

