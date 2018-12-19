#!/usr/bin/octave -qf

% This file is part of DynamicSimulator.
%
% (c) Franco Sauvisky <francosauvisky+ds@gmail.com>
%
% This source file is subject to the 3-Clause BSD License that is bundled
% with this source code in the file LICENSE.md

% ------------ Variables:

buffsize = 100; % frames
prebuffmax = 24; % perc
prebuffmin = 5; % perc
fpsmean = 20; % n de medidas

% ------------ Figure:

graphics_toolkit('qt');

function r = ternary (expr, true_val, false_val)
	if (expr)
		r = true_val;
	else
		r = false_val;
	endif
endfunction

function playpause(h)
	global pauseflag;
	if(pauseflag)
		set(h,'label','Pause');
		pauseflag = 0;
	else
		set(h,'label','Start');
		pauseflag = 1;
	endif
endfunction

function resetsim()
	global ctrlh outh sim_pid l h fcount rawdata;

	set(h,'cdata',ones(l));
	pause(0.001);

	fclose(ctrlh);
	fclose(outh);
	system(['kill -9 ' num2str(sim_pid)]);

	[ctrlh,outh,sim_pid] = popen2(argv(){end});

	clearfifo();
	rawdata = -1;
	fcount = 0;
	basetime = tic;
	frame = 0;
	pause(0.001);
endfunction

function superspeed()
	global speedflag superspeedval;
	if(speedflag == 1)
		speedflag = superspeedval;
	else
		speedflag = 1;
	endif
endfunction

function setspeed()
	global superspeedval;
	ssvalstring = inputdlg('Set Superspeed (updates per frame, default: 10):','Simulation Options');
	try
		superspeedval = str2double(ssvalstring{1});
	catch
		superspeedval = 10;
	end_try_catch

	basetime = tic;
	frame = 0;
	pause(0.001);
endfunction

function setfps()
	global fps frame basetime;
	fpsstring = inputdlg('Set FPS: (default: 24)','Simulation Options');
	try
		fps = str2double(fpsstring{1});
	catch
		fps = 24;
	end_try_catch

	basetime = tic;
	frame = 0;
	pause(0.001);
endfunction

f = figure('menubar','none','toolbar','figure','units','pixel','name','DynamicSimulator',...
	'numbertitle','off','closerequestfcn','killflag=1;','visible','off');

f1 = uimenu ('label', '&File','accelerator', 'f');
f11 = uimenu (f1, 'label', 'Autoscale', 'accelerator', 'a', ...
           'callback', 'global l;axis([0,l(1),0,l(2)])');
f11 = uimenu (f1, 'label', 'Close', 'accelerator', 'q', ...
           'callback', 'global killflag;killflag=1;');

f2 = uimenu ('label', '&Simulation','accelerator', 'a');
global f21 = uimenu (f2, 'label', 'Start', 'accelerator', 's',...
	'callback', 'global f21;playpause(f21);');
f22 = uimenu (f2, 'label', 'Superspeed', 'accelerator', 'v', ...
           'callback', 'superspeed();');
f23 = uimenu (f2, 'label', 'Reset', 'accelerator', 'r', ...
           'callback', 'resetsim();');

f3 = uimenu ('label', '&Options', 'accelerator', 's');
f31 = uimenu (f3, 'label', 'Set FPS','callback', 'setfps()');
f32 = uimenu (f3, 'label', 'Set Superspeed','callback', 'setspeed()');

global h = imshow(ones(1,1));

t1 = annotation('textbox',[0.02,0.02,0,0],'units','pixels',...
	'verticalalignment','bottom','horizontalalignment','left','linestyle','none');

% ------------ FIFO:

global fifo;
fifo = [];
fifo.data = cell(1,buffsize);
fifo.start = 0;
fifo.final = 0;
fifo.bsize = buffsize;

function b = pull()
	global fifo;
	if(fsize > 0)
		b = fifo.data{mod(fifo.start,fifo.bsize)+1};
		fifo.start = fifo.start + 1;
	else
		b = NaN;
	endif
endfunction

function push(b)
	global fifo;
	fifo.data{mod(fifo.final,fifo.bsize)+1} = b;
	fifo.final = fifo.final + 1;
endfunction

function n = fsize()
	global fifo;
	n = fifo.final - fifo.start;
endfunction

function clearfifo()
	global fifo;
	fifo.data{1} = [];
	fifo.start = 0;
	fifo.final = 0;
endfunction

function c = getdata(ind)
	global fifo;
	c = fifo.data(mod(ind,fifo.bsize)+1);
endfunction

function setdata(ind,data)
	global fifo;
	for i = ind
		fifo.data(mod(i,fifo.bsize)+1) = data(i);
	endfor
endfunction

% ------------ Code:

global basetime;
global frame = 0;
global fcount = 0;
global rawdata = -1;
global killflag = 0;
global pauseflag = 1;
global speedflag = 1;
global l;
global ctrlh outh sim_pid;
global prebuff = 0;
global fps = 24;
global superspeedval = 10;

olddata = [];

[ctrlh,outh,sim_pid] = popen2(argv(){end}); % open process

do % get size
	fprintf(ctrlh,'l'); fflush(ctrlh);
	l = fscanf(outh,'%d',2);
	fclear(outh);
	pause(0.1);
until l ~= -1
l = l(:)';

for i = 1:buffsize
	fifo.data{i} = zeros(l); % alocate buffer
end

axis([0,l(1),0,l(2)]);
set(f,'visible','on');
pause(0.001);

playpause(f21);
mfps = fps*ones(1,fpsmean);
fpstime = tic;
basetime = tic;

do
	if((fsize <= prebuffmin*buffsize/100) && (prebuff == 0)) % need prebuffer?
		prebuff = round(prebuffmax*buffsize/100) - fsize();
		title buffering;
		pause(0.001);

	elseif( ( (toc(basetime)*fps) > frame) && (prebuff == 0) && !pauseflag) % plot data
		frame = frame + 1;
		fcount = fcount + 1;

		% -- plotting:
		set(h,'cdata',pull());
		pause(0.001);

		mfps = [mfps(2:end) 1/toc(fpstime)];
		fpstime = tic;

		if( (toc(basetime)*fps) > frame)
			basetime = tic;
			frame = 0;
		endif
	endif

	if(isequal(rawdata, -1) && (fsize < buffsize-1)) % get data
		fprintf(ctrlh,'p'); fflush(ctrlh);
		pause(0.001);
		rawdata = fgetl(outh);
		fclear(outh);

	elseif(length(rawdata) < l(1)*l(2)) % clean bad data
		rawdata = -1;

	elseif((length(rawdata) >= l(1)*l(2)) && (fsize < buffsize-1)) % add data to buffer
		for i = 1:speedflag
			fprintf(ctrlh,'u');
		endfor
		fflush(ctrlh);

		try
			rawdata = reshape(rawdata == '+',l);
		catch
			disp('Bad data received!');
			rawdata = olddata;
		end_try_catch

		if(!isequal(olddata, rawdata)) % ignore if repeated
			push(rawdata);

			if(prebuff > 0)
				prebuff = prebuff - 1;
				if(prebuff == 0)
					title ' ';
					basetime = tic;
					frame = 0;
					pause(0.001);
				endif
			endif
		endif

		olddata = rawdata;
		rawdata = -1;
	endif

	if(pauseflag || (prebuff>0))
		basetime = tic;
		frame = 0;
		pause(0.001);
	endif

	set(t1,'string',[...
		ternary(pauseflag==0,'running','paused') "\n"...
		ternary(speedflag==1,'normal','superspeed') "\n"...
		'frame = ' num2str(fcount) "\n"...
		'fps = '  num2str(round(100*mean(mfps))/100) "\n"...
		'buffer = ' num2str(round(100*fsize/buffsize)) "\%\n"...
		'l = ' num2str(l)]);
until(killflag && ishandle(f))

% ------------ Close:

system(['kill -9 ' num2str(sim_pid)]);
