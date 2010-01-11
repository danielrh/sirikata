#!/usr/bin/python
import subprocess
import sys
import os

maxx=.1;
minx=None;
maxy=None
miny=None;
logPlot=False
loglogPlot=False

if len(sys.argv)==2 and sys.argv[1].find(".dat")!=-1 and os.path.exists(sys.argv[1]):
    f=open(sys.argv[1],"r")
    plotchartlines=f.readlines()
    f.close()
    title=sys.argv[1][0:sys.argv[1].find(".dat")]
    
else:
    (outp,inp)=os.pipe();
    sp = subprocess.Popen(['../../../build/cmake/queuetest']+sys.argv[1:], 0, None, None, inp, None, None, False, False, None, None)
    output=sp.communicate();
    plotchart=""
    os.close(inp);
    plotchartlines=os.fdopen(outp).readlines();
    title="queue";
    for argi in sys.argv[1:]:
        title+='_'+argi.replace("--","").replace("=","~").replace("queue","q").replace("knowledge","kno").replace("space","spc").replace("radius","rad").replace("stream","strm").replace("separate","sep").replace("false","f").replace("true","t").replace("distance","dist").replace("object","obj").replace("message","msg");
class DataClass:
    def __init__(self,title):
        self.title=title
        self.x=[]
        self.y=[]
dataClasses=[]
dataClass=None
f=open(title+'.dat','w');

shouldPass=maxx!=None
for s in plotchartlines:
    f.write(s)
    slist=s.split(',')
    if len(slist)==1:
        dataClass=DataClass(slist[0].rstrip().strip())
        dataClasses.append(dataClass)
    elif dataClass:
        x=float(slist[0])
        y=float(slist[1])
        dataClass.x.append(x)
        dataClass.y.append(y)
        if (maxx==None or x>maxx) and not shouldPass:
            maxx=x
        if minx==None or x<minx:
            minx=x
        if x<=maxx:
            if maxy==None or y>maxy:
                maxy=y
            if miny==None or y<miny:
                miny=y
f.close()
#for dataClass in dataClasses:
#    print dataClass.title
#    print dataClass.x
#    print dataClass.y
import matplotlib.pyplot as plt


from matplotlib import rcParams
rcParams.update( {
#'mathtext.sf' : 'serif',
#'font.family' : 'serif',
#'font.style' : 'normal',
#'font.variant' : 'normal',
#'font.weight' : 'bold',
#'font.stretch' : 'normal',
'font.serif':['Times New Roman'],
'font.size' : 16.0,
})


def set_axes_fontsize(axes, fontsize, fontfamily='serif'):
    even=1
    for tick in plt.gca().xaxis.get_major_ticks():
        tick.label1.set_fontsize(fontsize)
        tick.label1.set_fontname(fontfamily)
        if even:
            tick.label1.set_fontsize(0.01)
        even = not even;

    for tick in plt.gca().yaxis.get_major_ticks():
        tick.label1.set_fontsize(fontsize)
        tick.label1.set_fontname(fontfamily)

def set_legend_fontsize(legend, fontsize):
    if legend:
        for t in legend.get_texts():
            t.set_fontsize(fontsize)
            t.set_fontname('serif')


#fmts=[]
#max_fmt = 'bo'
#max_label = ' Max'

#avg_fmt = 'r^'
#avg_label = ' Avg'
formatSymbols=['D','o','^','.','<','>','p','*','h']#['-']
global_font_size=18.0;

plt.figure(1, figsize=(11,8.5), dpi=600)

plt.subplots_adjust(top=.96, bottom=.22, left=.14, right=.97)
index=0
for dataClass in dataClasses:
    plt.plot(dataClass.x,dataClass.y,formatSymbols[index%len(formatSymbols)],label=dataClass.title);
    index+=1

set_axes_fontsize(plt.gca(), global_font_size)
if logPlot or loglogPlot:
    plt.gca().set_xscale('log')
    if loglogPlot:
        plt.gca().set_yscale('log')
    else:
        pass
else:
    plt.axis([minx, maxx, 0, maxy])
plt.xlabel('priority',fontname='serif',fontsize=global_font_size)
plt.ylabel('bandwidth',fontname='serif',fontsize=global_font_size)
leg = plt.legend(loc='best')
set_legend_fontsize(leg, global_font_size)

plt.savefig(title+'.pdf')
os.execlp('xpdf','xpdf',title+'.pdf')
#plt.show()
#plt.show()
