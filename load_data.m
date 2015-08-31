
clear all


fid = fopen('C:\Users\Laptop\Documents\NS-3 Data\DefaultDelays.txt', 'rt');
default_delays = textscan(fid, 'Output Delay: %f', 'CollectOutput', true);
fclose(fid);

default_delays = cell2mat(default_delays);

fid = fopen('C:\Users\Laptop\Documents\NS-3 Data\InputDelays_5ms.txt', 'rt');
input_delays = textscan(fid, 'Input Delay: %f ms', 'CollectOutput', true);
fclose(fid);

input_delays = cell2mat(input_delays);

fid = fopen('C:\Users\Laptop\Documents\NS-3 Data\OutputDelays-2.txt', 'rt');
output_delays = textscan(fid, 'Output Delay: %f ms', 'CollectOutput', true);
fclose(fid);

output_delays = cell2mat(output_delays);

result = default_delays.*input_delays(1:2395);

figure(1)
hist(default_delays,20);
xlabel('Delay (ms)', 'fontsize', 14);
ylabel('Frequency', 'fontsize',14)
title('Default Delays Histogram', 'fontsize', 16);
grid on

figure(2)
hist(input_delays,20);
xlabel('Delay (ms)', 'fontsize', 14);
ylabel('Frequency', 'fontsize',14)
title('Input Delays Histogram', 'fontsize', 16);
grid on

figure(3)
hist(output_delays,20);
xlabel('Delay (ms)', 'fontsize', 14);
ylabel('Frequency', 'fontsize',14)
title('Output Delays Histogram', 'fontsize', 16);
grid on
