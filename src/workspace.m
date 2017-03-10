[wav, fs] = audioread('recording/72C.wav');

N = length(wav);
T = 1/fs;
t = (0:N-1) * T;

Y = fft(wav, N);

Y = abs(Y(1:N/2+1));
t = 0:(fs/N):(fs/2);

plot(0:3000, Y(1:3001));