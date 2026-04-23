load myDataNew.mat
cmap = jet(15);
figure(1); clf; hold on; grid on;
for p = 1:15
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
        for j = 1:64
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
