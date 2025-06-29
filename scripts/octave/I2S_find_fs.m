f_arr = zeros(1, 8001);
MCK_err_arr = zeros(1, 8001);
fs_err_arr = zeros(1, 8001);

i = 0;
for f = 8000:1:16000

  % User settings
  fs_req = f;
  source = 11289600;
  ratio = 384;
  MCK_req = ratio*fs_req;

  % Computations
  MCK_calc = 4096*floor((MCK_req*1048576)/(source+(MCK_req/2)));

  MCK_actual = source/floor((1048576*4096)/MCK_calc);
  MCK_err = 100*((MCK_actual-MCK_req)/MCK_req);

  fs_actual = MCK_actual/ratio;
  fs_err = 100*((fs_actual-fs_req)/fs_req);

  %Code settings
  source;
  ratio;
  MCK_calc;
  fs_req;

  % Errors
  MCK_err;
  fs_err;

  i = i + 1;
  f_arr(i) = f;
  MCK_err_arr(i) = MCK_err;
  fs_err_arr(i) = fs_err;

end

figure;
plot(f_arr, MCK_err_arr, 'r-', 'LineWidth', 2);
hold on;
plot(f_arr, fs_err_arr, 'b--', 'LineWidth', 2);
hold off;

xlabel('Requested sampling frequency fs (Hz)');
ylabel('Error (%)');
title('Relative error between requested and actual frequencies');
legend('MCK error', 'fs error', 'Location', 'northeast');
grid on;

idx = find(abs(fs_err_arr) < 1e-2);
f_arr(idx)

