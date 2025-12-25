% FIR High-Pass Filter Design
% Fs = 41670 Hz
% Cutoff: 500 Hz
% N = 31 tap (order = 30)

clear all;
close all;
clc;

Fs = 1000;
Fc = 50;      % cutoff frequency
N  = 63;       % number of taps
M  = N - 1;    % filter order

% Normalized radian cutoff
wc = 2 * pi * Fc / Fs;

% Preallocate
h_lp = zeros(1, N);   % low-pass prototype

% --- Ideal LPF at Fc ---
for n = 0:M
    if (n == M/2)
        h_lp(n+1) = wc / pi;
    else
        h_lp(n+1) = sin(wc * (n - M/2)) / (pi * (n - M/2));
    end
end

% High-pass = delta[n] - low-pass
h_hp = -h_lp;
h_hp(M/2 + 1) = 1 - h_lp(M/2 + 1);

% Apply Hamming window
w = hamming(N)';
h = h_hp .* w;

% Normalize to max amplitude
h = h / max(abs(h));

% Print coefficients in C format
printf("float HP_500HZ_IMPULSE_RESPONSE[%d] = {\n", N);
for i = 1:N
    if i < N
        printf("    %.9f,\n", h(i));
    else
        printf("    %.9f\n};\n", h(i));
    end
end

% Plot frequency response
figure;
freqz(h, 1, 4096, Fs);
title("FIR High-Pass 500 Hz, 31 tap, Fs=41670");

