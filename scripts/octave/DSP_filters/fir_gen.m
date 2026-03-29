function h = fir_gen(type, fs, order, fc1_hz, fc2_hz, header_filename, array_name, varargin)
  % fir_gen - Design FIR (windowed-sinc) + export coefficients to a C header (.h) as float32_t.
  %
  % USO:
  %   h = fir_gen('lowpass',  fs, order, fc1, [], 'fir_coeffs.h', 'fir_h');
  %   h = fir_gen('highpass', fs, order, fc1, [], 'fir_hp.h', 'fir_hp', 'window','kaiser','beta',10);
  %   h = fir_gen('bandpass', fs, order, f1,  f2, 'fir_bp.h', 'fir_bp');
  %   h = fir_gen('bandstop', fs, order, f1,  f2, 'fir_bs.h', 'fir_bs');
  %
  % PARAMETRI:
  %   type            : 'lowpass' | 'highpass' | 'bandpass' | 'bandstop'
  %   fs              : sampling frequency [Hz]
  %   order           : FIR order  => taps = order + 1
  %   fc1_hz, fc2_hz  : cutoff frequency/frequencies in Hz (fc2_hz required for bandpass/bandstop)
  %   header_filename : output header file name (.h)
  %   array_name      : C array name inside the header
  %
  % OPZIONI (name/value):
  %   'window'     : 'hamming'(default) | 'hann' | 'blackman' | 'rect' | 'kaiser'
  %   'beta'       : Kaiser beta (default 8.0) [only if window='kaiser']
  %   'normalize'  : true(default) -> normalize passband gain to ~1
  %   'dtype'      : 'single'(default) | 'double'
  %   'emit_meta'  : true(default) -> write metadata constants into header
  %
  % OUTPUT:
  %   h : FIR coefficients (column vector), single by default.

  if nargin < 7
    error('fir_gen requires at least 7 arguments.');
  endif

  % ---- defaults ----
  opt.window    = 'hamming';
  opt.beta      = 8.0;
  opt.normalize = true;
  opt.dtype     = 'single';
  opt.emit_meta = true;

  % ---- parse varargin (name/value pairs) ----
  if mod(numel(varargin), 2) ~= 0
    error('Optional args must be name/value pairs.');
  endif

  for k = 1:2:numel(varargin)
    key = lower(strtrim(varargin{k}));
    val = varargin{k+1};
    switch key
      case 'window'
        opt.window = lower(strtrim(val));
      case 'beta'
        opt.beta = val;
      case 'normalize'
        opt.normalize = logical(val);
      case 'dtype'
        opt.dtype = lower(strtrim(val));
      case 'emit_meta'
        opt.emit_meta = logical(val);
      otherwise
        error('Unknown option: %s', key);
    endswitch
  endfor

  % ---- design coefficients ----
  h = fir_design_internal(type, fs, order, fc1_hz, fc2_hz, opt);

  % ---- export to header ----
  meta = struct();
  meta.type = lower(strtrim(type));
  meta.fs = fs;
  meta.order = order;
  meta.taps = order + 1;
  meta.fc1_hz = fc1_hz;
  meta.fc2_hz = fc2_hz;
  meta.window = opt.window;
  meta.beta = opt.beta;
  meta.normalize = opt.normalize;

  write_c_float32_header(h, header_filename, array_name, meta, opt.emit_meta);

endfunction


% =========================================================================
% Internal: FIR design (windowed-sinc)
% =========================================================================
function h = fir_design_internal(type, fs, order, fc1_hz, fc2_hz, opt)

  type = lower(strtrim(type));

  assert(fs > 0, 'fs must be > 0');
  assert(order >= 1 && isfinite(order), 'order must be >= 1');

  taps = order + 1;

  % Suggerimento pratico:
  % - per filtri lineari in fase "comodi", usa taps dispari (=> order pari).
  % - con taps pari il "centro" cade tra due campioni (comunque funziona, ma meno classico).
  n = (0:taps-1).';
  mid = (taps-1)/2;   % can be .5 if taps even

  nyq = fs / 2;

  % Cast helper
  if strcmp(opt.dtype, 'single')
    castf = @(x) single(x);
  elseif strcmp(opt.dtype, 'double')
    castf = @(x) double(x);
  else
    error("dtype must be 'single' or 'double'");
  endif

  % Cutoffs checks + normalize to fs -> (0..0.5)
  if isempty(fc1_hz) || ~isfinite(fc1_hz)
    error('fc1_hz must be provided.');
  endif

  if any(strcmp(type, {'bandpass','bandstop'}))
    if isempty(fc2_hz) || ~isfinite(fc2_hz)
      error('fc2_hz must be provided for bandpass/bandstop.');
    endif
    f1 = min(fc1_hz, fc2_hz);
    f2 = max(fc1_hz, fc2_hz);
    assert(f1 > 0 && f2 > 0, 'Cutoffs must be > 0');
    assert(f2 < nyq, 'Cutoffs must be < fs/2');
    fc1 = f1 / fs;
    fc2 = f2 / fs;
  else
    assert(fc1_hz > 0 && fc1_hz < nyq, 'fc1_hz must be in (0, fs/2)');
    fc1 = fc1_hz / fs;
    fc2 = [];
  endif

  % Ideal lowpass: h[n] = 2*fc*sinc(2*fc*(n-mid))
  function h_lp = ideal_lowpass(fc)
    x = 2*fc*(n - mid);
    h_lp = 2*fc * sinc(x);  % Octave sinc(x) = sin(pi*x)/(pi*x)
  endfunction

  % Delta for spectral inversion
  % If taps odd: exact center; if taps even: choose "left center" index.
  idx0 = floor(mid) + 1;  % 1-based
  delta = zeros(taps, 1);
  delta(idx0) = 1;

  switch type
    case 'lowpass'
      h_ideal = ideal_lowpass(fc1);

    case 'highpass'
      h_ideal = delta - ideal_lowpass(fc1);

    case 'bandpass'
      h_ideal = ideal_lowpass(fc2) - ideal_lowpass(fc1);

    case 'bandstop'
      h_ideal = delta - (ideal_lowpass(fc2) - ideal_lowpass(fc1));

    otherwise
      error("Unknown type '%s'. Use lowpass/highpass/bandpass/bandstop.", type);
  endswitch

  % Window
  w = make_window(taps, opt.window, opt.beta);
  h = h_ideal(:) .* w(:);

  % Optional normalization (gain ~1 in passband)
  if opt.normalize
    nn = (0:taps-1).';

    if strcmp(type, 'lowpass') || strcmp(type, 'bandstop')
      w0 = 0;                % DC
    elseif strcmp(type, 'highpass')
      w0 = pi * 0.999999;    % near Nyquist (avoid edge numerical issues)
    else % bandpass
      fc_center = (fc1 + fc2)/2;
      w0 = 2*pi*fc_center;
    endif

    H0 = sum(h .* exp(-1j*w0*nn));
    g = abs(H0);
    if g > 0
      h = h / g;
    endif
  endif

  h = castf(h);

endfunction


% =========================================================================
% Window generator
% =========================================================================
function w = make_window(M, name, beta)

  n = (0:M-1).';
  name = lower(strtrim(name));

  switch name
    case {'rect','rectangular'}
      w = ones(M,1);

    case {'hann','hanning'}
      w = 0.5 - 0.5*cos(2*pi*n/(M-1));

    case 'hamming'
      w = 0.54 - 0.46*cos(2*pi*n/(M-1));

    case 'blackman'
      w = 0.42 - 0.5*cos(2*pi*n/(M-1)) + 0.08*cos(4*pi*n/(M-1));

    case 'kaiser'
      if isempty(beta), beta = 8.0; endif
      alpha = (M-1)/2;
      if alpha == 0
        w = ones(M,1);
      else
        t = (n - alpha) / alpha; % [-1,1]
        w = besseli(0, beta*sqrt(1 - t.^2)) / besseli(0, beta);
      endif

    otherwise
      error('Unknown window: %s', name);
  endswitch

  w = w(:);
endfunction


% =========================================================================
% Export: C header with float32_t array + optional metadata
% =========================================================================
function write_c_float32_header(x, header_filename, array_name, meta, emit_meta)

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
  fprintf(fid, '// Array: %s, Samples: %d\n\n', array_name, N);

  fprintf(fid, '#ifndef %s\n', guard);
  fprintf(fid, '#define %s\n\n', guard);

  fprintf(fid, '#include <stdint.h>\n\n');

  fprintf(fid, '/* If you use CMSIS-DSP, include "arm_math.h" or "cmsis_dsp.h" instead. */\n');
  fprintf(fid, '#ifndef __FLOAT32_T_DEFINED\n');
  fprintf(fid, '#define __FLOAT32_T_DEFINED\n');
  fprintf(fid, 'typedef float float32_t;\n');
  fprintf(fid, '#endif\n\n');

  if emit_meta && isstruct(meta)
    fprintf(fid, '/* FIR design metadata */\n');
    fprintf(fid, 'static const uint32_t %s_taps  = %u;\n', array_name, meta.taps);
    fprintf(fid, 'static const uint32_t %s_order = %u;\n', array_name, meta.order);
    fprintf(fid, 'static const float32_t %s_fs_hz  = %.9gf;\n', array_name, double(meta.fs));
    fprintf(fid, 'static const float32_t %s_fc1_hz = %.9gf;\n', array_name, double(meta.fc1_hz));
    if ~isempty(meta.fc2_hz)
      fprintf(fid, 'static const float32_t %s_fc2_hz = %.9gf;\n', array_name, double(meta.fc2_hz));
    else
      fprintf(fid, 'static const float32_t %s_fc2_hz = %.9gf; /* unused */\n', array_name, 0.0);
    endif
    fprintf(fid, '/* type=%s, window=%s, beta=%.3f, normalize=%d */\n\n', ...
            meta.type, meta.window, double(meta.beta), meta.normalize);
  endif

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
