#!/usr/bin/env python3
import pathlib,subprocess,json,random,statistics,os

import argparse,tempfile,shutil,shlex,platform
parser=argparse.ArgumentParser(description="Paired performance comparison; build both libraries with matching flags first.")
parser.add_argument('--baseline',type=pathlib.Path,required=True)
parser.add_argument('--candidate',type=pathlib.Path,required=True)
parser.add_argument('--out',type=pathlib.Path,required=True)
parser.add_argument('--pairs',type=int,default=10)
parser.add_argument('--target-seconds',type=float,default=0.25)
parser.add_argument('--cxx',default='g++')
parser.add_argument('--cxxflags',default='-Wall -Wextra -Werror -std=c++98 -O2 -g -fno-omit-frame-pointer')
args=parser.parse_args()
if args.pairs<2 or args.pairs%2 or args.target_seconds<=0:parser.error('pairs must be even and >=2; target-seconds must be positive')
args.baseline=args.baseline.resolve();args.candidate=args.candidate.resolve()
out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
source=pathlib.Path(__file__).resolve().parent/'driver.cpp'
scratch=tempfile.TemporaryDirectory(prefix='rug-performance-')
root=pathlib.Path(scratch.name);(root/'fixtures').mkdir();(root/'bench').mkdir()
shutil.copy2(source,root/'bench/driver.cpp')
for name,n in [('tiny',1),('medium',32),('large',128)]:
 lines=['start = '+' '.join('r'+str(i) for i in range(n))]+['r'+str(i)+' = %x'+format(65+i%26,'02X') for i in range(n)]
 (root/'fixtures'/(name+'.abnf')).write_text('\n'.join(lines)+'\n')
 (root/'fixtures'/(name+'.valid')).write_bytes(bytes(65+i%26 for i in range(n)))
(root/'fixtures/binary.abnf').write_text('start = *octet\noctet = %x00-FF\n')
fixture_rng=random.Random(5234)
(root/'fixtures/binary.valid').write_bytes(bytes(fixture_rng.randrange(256) for _ in range(4096)))
for name in ['calculator','ticket']:
 shutil.copy2(args.baseline/'examples'/name/(name+'.abnf'),root/'fixtures'/(name+'.abnf'))
(root/'fixtures/calculator.valid').write_text(' + '.join(['1 + 2 * (3 - 4)']*64))
shutil.copy2(args.baseline/'examples/ticket/db.txt',root/'fixtures/ticket.valid')
for variant,path in [('baseline',args.baseline),('construction',args.candidate)]:
 binary=root/'work'/variant/'construct';binary.parent.mkdir(parents=True)
 command=[args.cxx]+shlex.split(args.cxxflags)+['-I'+str(path),str(source),str(path/'librug.a'),'-o',str(binary)]
 subprocess.run(command,check=True)
(out/'environment.json').write_text(json.dumps({'platform':platform.platform(),'compiler':subprocess.check_output([args.cxx,'--version'],text=True),'flags':args.cxxflags,'baseline':str(args.baseline),'candidate':str(args.candidate),'pairs':args.pairs,'target_seconds':args.target_seconds},indent=2)+'\n')

rng=random.Random(5234);rows=[];summaries=[]
cpu=min(os.sched_getaffinity(0))
def run(variant,name,loops):
 cmd=['taskset','-c',str(cpu),str(root/'work'/variant/'construct'),str(root/'fixtures'/(name+'.abnf')),str(loops)]
 r=subprocess.run(cmd,capture_output=True,text=True,timeout=30);r.check_returncode()
 row={'variant':variant,'dataset':name,'loops':loops,'command':cmd,'exit':r.returncode,'metrics':json.loads(r.stdout),'stderr':r.stderr}
 rows.append(row);return row
for name in ['tiny','medium','large','binary','calculator','ticket']:
 pilot=run('baseline',name,3);pilot['phase']='calibration'
 loops=max(3,min(2000,int(3*args.target_seconds/max(pilot['metrics']['build_seconds'],1e-6))))
 for warmup in range(2):
  for variant in ['baseline','construction']:run(variant,name,loops).update(phase='warmup',trial=warmup)
 ratios=[];values={v:[] for v in ['baseline','construction']};orders=[['baseline','construction']]*(args.pairs//2)+[['construction','baseline']]*(args.pairs//2);rng.shuffle(orders)
 for trial,order in enumerate(orders):
  pair={}
  for v in order:
   row=run(v,name,loops);row.update(phase='measure',trial=trial,order=order);pair[v]=row['metrics'];values[v].append(pair[v]['build_seconds']/loops)
  assert pair['baseline']['image_bytes_sum']==pair['construction']['image_bytes_sum']
  ratios.append(pair['construction']['build_seconds']/pair['baseline']['build_seconds'])
  (out/'construction-raw.json').write_text(json.dumps(rows,indent=2)+'\n')
 boot=sorted(statistics.median(rng.choices(ratios,k=args.pairs)) for _ in range(5000))
 item={'dataset':name,'loops':loops,'pairs':args.pairs,'ratio':statistics.median(ratios),'ratio_bootstrap_95':[boot[124],boot[4874]],'paired_ratios':ratios}
 for v,times in values.items():
  med=statistics.median(times);item[v]={'seconds_per_build_median':med,'seconds_per_build_mad':statistics.median(abs(t-med) for t in times),'min':min(times),'max':max(times)}
 summaries.append(item);(out/'construction-summary.json').write_text(json.dumps(summaries,indent=2)+'\n')
 print(name,'construction speedup',round(1/item['ratio'],2),'x',flush=True)
