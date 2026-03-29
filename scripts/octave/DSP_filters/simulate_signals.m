function simulate_signals()
  % simulate_signals.m
  % Single-file Octave program to simulate signals and export to a C header.
  % Language: English

  clear; close all; clc;

  % ---------------------------
  % Example parameters
  % ---------------------------
  fs         = 44100;       % Sampling frequency [Hz]
  A          = 0.8;         % Amplitude (peak)
  duration_s = 0.01;        % Duration [s]
  phase_rad  = 0;           % Phase shift [rad]

  % Fixed-frequency examples
  f0   = 1000;              % Sine frequency [Hz]
  duty = 0.50;              % Duty cycle for square wave (0..1)

  % Chirp (variable-frequency sine) parameters
  f_start = 300;            % Start frequency [Hz]
  f_end   = 6000;           % End frequency [Hz]
  sweep_type = 'linear';    % 'linear' (supported). Easy to extend later.

  % ---------------------------
  % Generate signals (float32 / single)
  % ---------------------------
  [x_sin, t]   = generate_sine(fs, f0, A, duration_s, phase_rad);

  % NEW: Variable-frequency sine (chirp)
  [x_chirp, ~] = generate_chirp_sine(fs, f_start, f_end, A, duration_s, phase_rad, sweep_type);

  % ---------------------------
  % Add white Gaussian noise
  % ---------------------------
  x = zeros(1, numel(x_sin));
  [~, n] = add_awgn(x, 'sigma', 50);

  % ---------------------------
  % Filter Gaussian noise to obtain low frequency noise
  % ---------------------------
  fir_order = 1000;
  ft = 50; % [Hz]
  h_lpf = fir_gen('lowpass',  fs, fir_order, ft, [], 'fir_coeffs.h', 'fir_h');
  n_lf = filter(h_lpf, 1, n);

  % ---------------------------
  % Add low frequency nois to the signal
  % ---------------------------
  x_sin_n = (x_sin + n_lf);
  x_sin_n = (x_sin_n./max(x_sin_n));

  % ---------------------------
  % Apply FIR filter to the noisy signal
  % ---------------------------
  fir_order = 39;
  ft = 19000; % [Hz]
  h_hpf = fir_gen('lowpass',  fs, fir_order, ft, [], 'fir_coeffs.h', 'fir_h');
  x_sin_filt = filter(h_hpf, 1, x_sin_n);

  % ---------------------------
  % Plot signals
  % ---------------------------
  plot_signal(x_sin,   t, 'Sine (clean)', 'Time [s]', 'Amplitude');
  plot_signal(n_lf, t, 'Low frequency noise', 'Time [s]', 'Amplitude');
  plot_signal(x_sin_n,   t, 'Sine (noisy)', 'Time [s]', 'Amplitude');
  plot_signal(x_sin_filt, t, 'Sine (filtered)', 'Time [s]', 'Amplitude');

  % ---------------------------
  % Export to a C header (.h)
  % ---------------------------
  header_filename = 'signal.h';
  array_name      = 'signal';

  write_c_float32_header(x_sin, header_filename, array_name);
  fprintf('Generated header: %s (array name: %s, samples: %d)\n', ...
          header_filename, array_name, numel(x_sin));

endfunction


function [x, t] = generate_sine(fs, f0, A, duration_s, phase_rad)
  % generate_sine - Generate a sine wave (float32).

  if nargin < 5
    phase_rad = 0;
  endif

  assert(fs > 0, 'fs must be > 0');
  assert(duration_s > 0, 'duration_s must be > 0');

  N = max(1, round(duration_s * fs));

  t = single((0:N-1) / fs);
  x = single(A) .* sin(single(2*pi*f0) .* t + single(phase_rad));
endfunction


function [x, t] = generate_square(fs, f0, A, duration_s, duty, phase_rad)
  % generate_square - Generate a square wave in [-A, +A] (float32).

  if nargin < 6
    phase_rad = 0;
  endif
  if nargin < 5
    duty = 0.5;
  endif

  assert(fs > 0, 'fs must be > 0');
  assert(duration_s > 0, 'duration_s must be > 0');
  assert(duty > 0 && duty < 1, 'duty must be in (0, 1)');

  N = max(1, round(duration_s * fs));

  t = single((0:N-1) / fs);

  phase_cycles = single(phase_rad / (2*pi));
  p = mod(single(f0) .* t + phase_cycles, single(1));     % [0,1)
  x = single(A) .* single(2*(p < single(duty)) - 1);
endfunction


function [x, t] = generate_triangle(fs, f0, A, duration_s, phase_rad)
  % generate_triangle - Generate a triangle wave in [-A, +A] (float32).

  if nargin < 5
    phase_rad = 0;
  endif

  assert(fs > 0, 'fs must be > 0');
  assert(duration_s > 0, 'duration_s must be > 0');

  N = max(1, round(duration_s * fs));

  t = single((0:N-1) / fs);

  phase_cycles = single(phase_rad / (2*pi));
  p = mod(single(f0) .* t + phase_cycles, single(1));   % [0,1)
  x = single(A) .* (single(4) .* abs(p - single(0.5)) - single(1));
endfunction


function [x, t] = generate_chirp_sine(fs, f_start, f_end, A, duration_s, phase_rad, sweep_type)
  % generate_chirp_sine - Generate a variable-frequency sine (chirp), float32.
  %
  % Inputs:
  %   fs         : sampling frequency [Hz]
  %   f_start    : start frequency [Hz]
  %   f_end      : end frequency [Hz]
  %   A          : amplitude (peak)
  %   duration_s : duration [s]
  %   phase_rad  : initial phase [rad] (optional, default 0)
  %   sweep_type : 'linear' (optional, default 'linear')
  %
  % Output:
  %   x : chirp samples (single)
  %   t : time vector (single)

  if nargin < 6
    phase_rad = 0;
  endif
  if nargin < 7 || isempty(sweep_type)
    sweep_type = 'linear';
  endif

  assert(fs > 0, 'fs must be > 0');
  assert(duration_s > 0, 'duration_s must be > 0');
  assert(f_start >= 0 && f_end >= 0, 'f_start and f_end must be >= 0');

  N = max(1, round(duration_s * fs));
  t = single((0:N-1) / fs);

  T = single(duration_s);
  A = single(A);
  phase0 = single(phase_rad);

  if strcmpi(sweep_type, 'linear')
    % Linear chirp:
    % f(t) = f_start + k*t, k = (f_end - f_start)/T
    % phi(t) = 2*pi*(f_start*t + 0.5*k*t^2) + phase0
    k = single((f_end - f_start) / duration_s);
    phi = single(2*pi) .* (single(f_start).*t + single(0.5).*k.*(t.^2)) + phase0;
    x = A .* sin(phi);

  else
    error("Unsupported sweep_type. Use 'linear'.");
  endif
endfunction


function [y, n] = add_awgn(x, mode, value, seed)
  % add_awgn - Add AWGN, float32 output.
  %
  % Usage:
  %   [y, n] = add_awgn(x, 'sigma',  sigma,  seed_optional)
  %   [y, n] = add_awgn(x, 'snr_db', snr_db, seed_optional)

  if nargin < 3
    error("add_awgn requires at least (x, mode, value)");
  endif

  if nargin >= 4 && ~isempty(seed)
    randn('seed', seed);
    rand('seed', seed);
  endif

  was_row = isrow(x);
  x = single(x);
  xcol = x(:);
  N = numel(xcol);

  if strcmpi(mode, 'sigma')
    sigma = single(value);
    assert(sigma >= 0, 'sigma must be >= 0');

  elseif strcmpi(mode, 'snr_db')
    snr_db = double(value);
    Px = mean(double(abs(xcol)).^2);
    if Px == 0
      error('Signal power is zero; cannot set noise from SNR.');
    endif
    Pn = Px / (10^(snr_db/10));
    sigma = single(sqrt(Pn));

  else
    error("Unknown mode. Use 'sigma' or 'snr_db'.");
  endif

  ncol = sigma .* single(randn(N, 1));
  ycol = xcol + ncol;

  if was_row
    y = ycol.';
    n = ncol.';
  else
    y = ycol;
    n = ncol;
  endif
endfunction


function plot_signal(x, varargin)
  % plot_signal - Generic plotting utility (reusable for any signal).
  %
  % Usage:
  %   plot_signal(x)                          -> x vs sample index
  %   plot_signal(x, fs)                      -> x vs time using fs
  %   plot_signal(x, t)                       -> x vs provided axis vector
  %   plot_signal(x, t, titleStr)
  %   plot_signal(x, t, titleStr, xlab, ylab)

  if nargin < 1
    error('plot_signal requires at least one input: x');
  endif

  x = x(:).';

  axis_vals = 1:numel(x);
  xLabelStr = 'Sample index';
  yLabelStr = 'Amplitude';
  titleStr  = 'Signal';

  argi = 1;

  if numel(varargin) >= 1
    a1 = varargin{1};

    if isscalar(a1) && isnumeric(a1) && a1 > 0
      fs = a1;
      axis_vals = (0:numel(x)-1) / fs;
      xLabelStr = 'Time [s]';
      argi = 2;
    elseif isnumeric(a1) && isvector(a1) && numel(a1) == numel(x)
      axis_vals = a1(:).';
      xLabelStr = 'Time [s]';
      argi = 2;
    endif
  endif

  if numel(varargin) >= argi && ischar(varargin{argi})
    titleStr = varargin{argi};
    argi = argi + 1;
  endif
  if numel(varargin) >= argi && ischar(varargin{argi})
    xLabelStr = varargin{argi};
    argi = argi + 1;
  endif
  if numel(varargin) >= argi && ischar(varargin{argi})
    yLabelStr = varargin{argi};
    argi = argi + 1;
  endif

  figure;
  if ~isreal(x)
    plot(axis_vals, real(x), 'LineWidth', 1.2);
    yLabelStr = [yLabelStr ' (real part)'];
  else
    plot(axis_vals, x, 'LineWidth', 1.2);
  endif

  grid on;
  xlabel(xLabelStr);
  ylabel(yLabelStr);
  title(titleStr);
endfunction


function write_c_float32_header(x, header_filename, array_name)
  % write_c_float32_header - Generate a .h file containing a float32_t array.

  if nargin < 3
    error('write_c_float32_header requires (x, header_filename, array_name)');
  endif

  x = single(x(:));
  N = numel(x);

  guard = upper(regexprep(header_filename, '[^a-zA-Z0-9]', '_'));
  guard = [guard '_'];

  fid = fopen(header_filename, 'w');
  if fid < 0
    error('Cannot open file for writing: %s', header_filename);
  endif

  fprintf(fid, '// Auto-generated by Octave\n');
  fprintf(fid, '// Samples: %d\n\n', N);

  fprintf(fid, '#ifndef %s\n', guard);
  fprintf(fid, '#define %s\n\n', guard);

  fprintf(fid, '#include <stdint.h>\n\n');

  fprintf(fid, '/* If you use CMSIS-DSP, include "arm_math.h" or "cmsis_dsp.h" instead. */\n');
  fprintf(fid, '#ifndef __FLOAT32_T_DEFINED\n');
  fprintf(fid, '#define __FLOAT32_T_DEFINED\n');
  fprintf(fid, 'typedef float float32_t;\n');
  fprintf(fid, '#endif\n\n');

  fprintf(fid, 'static const uint32_t %s_len = %u;\n', array_name, N);
  fprintf(fid, 'static const float32_t %s[%u] = {\n', array_name, N);

  values_per_line = 8;

  for i = 1:N
    val_str = sprintf('%.9gf', double(x(i)));

    if mod(i-1, values_per_line) == 0
      fprintf(fid, '  ');
    endif

    if i < N
      fprintf(fid, '%s, ', val_str);
    else
      fprintf(fid, '%s', val_str);
    endif

    if mod(i, values_per_line) == 0 || i == N
      fprintf(fid, '\n');
    endif
  endfor

  fprintf(fid, '};\n\n');
  fprintf(fid, '#endif // %s\n', guard);

  fclose(fid);
endfunction

