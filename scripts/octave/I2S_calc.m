% User settings
fs_req = 8000;
source = 32000000;
ratio = 384;
MCK_req = ratio*fs_req;

% Computations
MCK_calc = 4096*floor((MCK_req*1048576)/(source+(MCK_req/2)));

MCK_actual = source/floor((1048576*4096)/MCK_calc)
MCK_err = 100*((MCK_actual-MCK_req)/MCK_req);

fs_actual = MCK_actual/ratio;
fs_err = 100*((fs_actual-fs_req)/fs_req);

%Code settings
source
ratio
MCK_calc_hex = dec2hex(MCK_calc)
fs_req

% Errors
MCK_err
fs_err
