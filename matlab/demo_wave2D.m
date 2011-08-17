
h = .01;
[x y] = meshgrid(-1:h:1, -1:h:1 );

u0 = 1.0 * ( x > 0.75 );
u0 = imfilter( u0, fspecial('gaussian', 5, 2 ),'replicate' ); % smooth with 5x5 guassian sigma = 2
surf( x, y , u0 ); title('initial value');

uprv0 = u0;

dt = h / 10;
Tstop = 1;
t = 0;

peekL = @(u)  [ u(:,1) , u(:,1:end-1) ];
peekR = @(u)  [ u(:,2:end) ,  u(:,end) ];
peekU = @(u)  [ u(1,:) ; u(1:end-1,:) ];
peekD = @(u)  [ u(2:end,:) ; u(end,:) ];

u    = u0;
uprv = uprv0;

while ( t < Tstop )
   t = t + dt;
   
   u_next = 2 * u + dt^2/h^2 * ( peekL(u) + peekR(u) + peekD(u) + peekU(u) - 4 * u ) - uprv;
   
   uprv = u;
   u    = u_next;
   surf(u); xlabel('x'); ylabel('y'); zlabel('u(x,y,t)'); title(['plot of u(x,y,t) at t = ' num2str(t)]);
   drawnow;
   breakhere = 1;
   
end