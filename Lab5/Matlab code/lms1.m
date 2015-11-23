%noise cancellation

clear
close all
c=@(x) round(2^16*x)/2^16;
order=10;

size=2;                         %time duration of inputs
fs=8192;                                %digital sampling frequency
t=[0:1/fs:size];
N=fs*size+1;                      %size of inputs
f1=3500/2;                                %frequency of voice
f2=99/2;                                %frequency of noise

voice=cos(2*pi*f1*t);
[voice, fs] =wavread('C:\Users\User\Downloads\troll_8.wav');
voice=load('handel');
fs=voice.Fs;
voice=voice.y';
% voice=voice(1:fs*10)';
% voice=c(voice);
N=length(voice);
t=[0:1/fs:(N-1)/fs];
% voice=zeros(1,N);
subplot(4,1,1)
plot(t,voice);
title('voice    (don''t have access to)')

noise=cos(2*pi*f2.*t.^2);                                %increasy frequency noise

%noise=rand(1,N);                %white noise
noise=c(noise);
primary=voice+0.5*circshift(noise,[0 -round(0.1*fs)]);
primary=c(primary);
subplot(4,1,2)
plot(t,primary)
title('primary = voice + noise   (input1)')

ref=noise+0.1*rand(1,N);                                             %noisy noise
ref=c(ref);
subplot(4,1,3)
plot(t,ref)
title('reference  (noisy noise)   (input2)');

w=zeros(order,N);
mu=1;

for i=order:N
   buffer = ref(i-order+1:i);                                   %current 32 points of reference
   desired(i) = c(primary(i)-buffer*w(:,i-1));                    %dot product reference and coeffs
   %mu=(primary(i)-buffer*w(:,i-1))^2/desired(i)^2;
   w(:,i)=c(w(:,i-1)+(buffer.*mu*desired(i)/(buffer*buffer'+0.000001))');%update coeffs
end

subplot(4,1,4)
plot(t(order:N),desired(order:N))
title('Adaptive output  (hopefully it''s close to "voice")')

