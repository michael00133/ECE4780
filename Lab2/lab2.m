
close all
L = 100;             % Length of signal

hold all
for i=1:0.5:4
Fs = 1000*i;            % Sampling frequency
T = 1/Fs;             % Sampling period
t=(0:L-1)*T;
S=sin(2*pi*1477*t);
Noise=0.5*rand(size(t));


%plot(t,S)

X=S+Noise;
Y=fft(S);
YN=fft(Noise);
P2 = abs(Y/L);
P1 = P2(1:L/2+1);
P1(2:end-1) = 2*P1(2:end-1);
P4 = abs(YN/L);
P3 = P4(1:L/2+1);
P3(2:end-1) = 2*P3(2:end-1);
f = Fs*(0:(L/2))/L;
plot(f,20*log10(P1))
end
figure
plot(f,20*log10(P3))
title('Single-Sided Amplitude Spectrum of X(t)')
xlabel('f (Hz)')
ylabel('|P1(f)|')


%SNR=max(20*log10(P1))+mean(20*log10(P3))