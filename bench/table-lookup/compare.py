#!/usr/bin/env python3
"""Interleaved paired unprofiled trials; CPU affinity inherited by children only."""
import pathlib,json,subprocess,statistics,random,os,hashlib,time

import argparse,tempfile,shutil,shlex,platform
parser=argparse.ArgumentParser(description="Paired performance comparison; build both libraries with matching flags first.")
parser.add_argument('--baseline',type=pathlib.Path,required=True)
parser.add_argument('--candidate',type=pathlib.Path,required=True)
parser.add_argument('--out',type=pathlib.Path,required=True)
parser.add_argument('--pairs',type=int,default=10)
parser.add_argument('--target-seconds',type=float,default=0.20)
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
for variant,path in [('baseline',args.baseline),('candidate',args.candidate)]:
 binary=root/'work'/variant/'driver';binary.parent.mkdir(parents=True)
 command=[args.cxx]+shlex.split(args.cxxflags)+['-I'+str(path),str(source),str(path/'librug.a'),'-o',str(binary)]
 subprocess.run(command,check=True)
(out/'environment.json').write_text(json.dumps({'platform':platform.platform(),'compiler':subprocess.check_output([args.cxx,'--version'],text=True),'flags':args.cxxflags,'baseline':str(args.baseline),'candidate':str(args.candidate),'pairs':args.pairs,'target_seconds':args.target_seconds},indent=2)+'\n')

rng=random.Random(5234);cpu=min(os.sched_getaffinity(0))
rows=[];summary=[]
def run(variant,name,chunk,loops):
 cmd=['taskset','-c',str(cpu),str(root/'work'/variant/'driver'),str(root/'fixtures'/(name+'.abnf')),str(root/'fixtures'/(name+'.valid')),str(chunk),str(loops),'1']
 r=subprocess.run(cmd,capture_output=True,text=True,timeout=30)
 row={'variant':variant,'dataset':name,'chunk':chunk,'loops':loops,'command':cmd,'exit':r.returncode,'stderr':r.stderr,'stdout':r.stdout}
 if r.returncode==0:row['metrics']=json.loads(r.stdout)
 else:raise RuntimeError(row)
 return row
shapes=[(name,chunk) for name in ['tiny','medium','large','binary','calculator','ticket'] for chunk in [1,7,4096]]
rng.shuffle(shapes)
for name,chunk in shapes:
 pilot=run('baseline',name,chunk,1000);pilot['phase']='calibration';rows.append(pilot)
 loops=max(100,min(2000000,int(1000*args.target_seconds/max(pilot['metrics']['parse_seconds'],1e-6))))
 for warmup in range(2):
  for variant in ['baseline','candidate']:
   row=run(variant,name,chunk,loops);row.update(phase='warmup',trial=warmup);rows.append(row)
 pairs=[];orders=[['baseline','candidate']]*(args.pairs//2)+[['candidate','baseline']]*(args.pairs//2);rng.shuffle(orders)
 for trial,order in enumerate(orders):
  pair={}
  for variant in order:
   row=run(variant,name,chunk,loops);row.update(phase='measure',trial=trial,order=order);rows.append(row);pair[variant]=row['metrics']
  assert pair['baseline']['grammar_image_bytes']==pair['candidate']['grammar_image_bytes']
  pairs.append(pair)
  (out/'paired-raw.json').write_text(json.dumps(rows,indent=2)+'\n')
 ratios=[p['candidate']['parse_seconds']/p['baseline']['parse_seconds'] for p in pairs]
 boot=[statistics.median(rng.choices(ratios,k=len(ratios))) for _ in range(5000)];boot.sort()
 item={'dataset':name,'chunk':chunk,'loops':loops,'pairs':args.pairs,'ratio_candidate_over_baseline':statistics.median(ratios),'ratio_bootstrap_95':[boot[124],boot[4874]],'paired_ratios':ratios}
 for variant in ['baseline','candidate']:
  times=[p[variant]['parse_seconds'] for p in pairs];med=statistics.median(times)
  item[variant]={'parse_median_seconds':med,'parse_mad_seconds':statistics.median(abs(t-med) for t in times),'parse_min_seconds':min(times),'parse_max_seconds':max(times),'ns_per_byte':med/(loops*pairs[0][variant]['input_bytes'])*1e9,'image_bytes':pairs[0][variant]['grammar_image_bytes']}
 summary.append(item);(out/'paired-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
 print(name,chunk,'parse time reduction',round((1-item['ratio_candidate_over_baseline'])*100,1),'%',flush=True)
metadata={'seed':5234,'cpu':cpu,'method':'balanced paired order, 2 warmups each, 5000 paired median bootstrap resamples','load_end':os.getloadavg(),'driver_sha256':hashlib.sha256((root/'bench/driver.cpp').read_bytes()).hexdigest()}
(out/'paired-method.json').write_text(json.dumps(metadata,indent=2)+'\n')
