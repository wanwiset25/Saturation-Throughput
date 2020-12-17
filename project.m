clear;

n = 1:50;
w = 32;
m = 3;
T1=[];
for i = 1:length(n)
    T1 = [T1 throughput(n(i),w,m)];
end

w = 32;
m = 5;
T2=[];
for i = 1:length(n)
    T2 = [T2 throughput(n(i),w,m)];
end

w = 128;
m = 3;
T3=[];
for i = 1:length(n)
    T3 = [T3 throughput(n(i),w,m)];
end

%dataset from NS-3 simulation @payloadSize = 995; @simulationTime=100;
x = [1 5 10 15 20 30 50];
y1 = [0.834865 0.800329 0.74255 0.702694 0.668894 0.61578 0.537541];
y2 = [0.832983 0.804012 0.74877 0.716607 0.683134 0.647861 0.599166];
y3 = [0.673886 0.816615 0.817106 0.801475 0.788626 0.761864 0.715788];

f = figure;
ylim([0.5 0.9]);
hold on;
grid on;

plot(n, T1,'k');
plot(x,y1,'k*');
plot(n, T2,'b');
plot(x,y2,'b+');
plot(n, T3,'r');
plot(x,y3,'rx');

title('802.11 DCF Saturation Throughput');
ylabel('Saturation Throughput (Max = 1 Mbps)');
xlabel('n = Number of stations');
legend('W=32, m=3', 'W=32, m=3','W=32, m=5', 'W=32, m=5','W=128, m=3', 'W=128, m=3');

%error calculation: difference between points and line
E1 = ([T1(1) T1(5) T1(10) T1(15) T1(20) T1(30) T1(50)] - y1)./[T1(1) T1(5) T1(10) T1(15) T1(20) T1(30) T1(50)]*100;
E2 = ([T2(1) T2(5) T2(10) T2(15) T2(20) T2(30) T2(50)] - y2)./[T2(1) T2(5) T2(10) T2(15) T2(20) T2(30) T2(50)]*100;
E3 = ([T3(1) T3(5) T3(10) T3(15) T3(20) T3(30) T3(50)] - y3)./[T3(1) T3(5) T3(10) T3(15) T3(20) T3(30) T3(50)]*100;
Eavg = (sum(E1)+sum(E2)+sum(E3))/(length(E1)+length(E2)+length(E3));

%%code for RTS CTS case. Not used.
% w = 32;
% m = 3;
% T4=[];
% for i = 1:length(n)
%     T4 = [T4 throughputRTS(n(i),w,m)];
% end
% plot(n, T4);
% 
% w = 128;
% m = 3;
% T5=[];
% for i = 1:length(n)
%     T5 = [T5 throughputRTS(n(i),w,m)];
% end
% plot(n, T5);

function T = throughput(n, w, m)

pkt = 8184;
phy = 128;
mac = 272;
ack = 112+phy;
sifs = 28;
difs = 128;
P_d = 1;
slot = 50;

options = optimset('Display','off');

f = @(x)[-x(1)+1-(1-x(2))^(n-1);  -x(2)+(2*(1-2*x(1)))/((1-2*x(1))*(w+1)+x(1)*w*(1-(2*x(1))^m))];
[x, fval] = fsolve(f, [0.05,0.01],options);
p = x(1);
tau = x(2);

P_tr = 1-(1-tau)^n;
P_s = (n*tau*(1-tau)^(n-1))/P_tr;
E_phi = 1/P_tr -1;
H = phy + mac;
bits_s = H + pkt + sifs + P_d + ack + difs + P_d;
bits_c = H + pkt + difs + P_d;
T_s = bits_s/slot;
T_c = bits_c/slot;
T = (P_s*pkt)/(E_phi+P_s*T_s+(1-P_s)*T_c)/slot;

end

%function for RTS/CTS case. Not used.
function T = throughputRTS(n, w, m)

pkt = 8184;
phy = 128;
mac = 272;
ack = 112+phy;
sifs = 28;
difs = 128;
rts = 160+phy;
cts = 112+phy;
P_d = 1;
slot = 50;

options = optimset('Display','off');

f = @(x)[-x(1)+1-(1-x(2))^(n-1);  -x(2)+(2*(1-2*x(1)))/((1-2*x(1))*(w+1)+x(1)*w*(1-(2*x(1))^m))];
[x, fval] = fsolve(f, [0.001,0.001],options);
p = x(1);
tau = x(2);

P_tr = 1-(1-tau)^n;
P_s = (n*tau*(1-tau)^(n-1))/P_tr;
E_phi = 1/P_tr -1;
H = phy + mac;
bits_s = rts + sifs + P_d + cts + sifs + P_d + H + pkt + sifs + P_d + ack + difs + P_d;
bits_c = rts + difs + P_d;
T_s = bits_s/slot;
T_c = bits_c/slot;
T = (P_s*pkt)/(E_phi+P_s*T_s+(1-P_s)*T_c)/slot;

end





