"""Script function signatures of the Black Plague and Requiem binaries.

Reads the argument types of every script function from the wrappers the
binaries export (objdump), compares them with GameScripts.cpp and writes the
ones we do not have to the JSON file given. int and bool arguments look the
same in the binary; the scripts' own calls tell them apart.

    python3 tools/porting/inspect_script_signatures.py out.json
"""
import subprocess,re,sys,collections,json,os
GAME_BINARY_DIR=os.path.expanduser("~/.local/share/Steam/steamapps/common/Penumbra Black Plague/")
SCRIPT_SOURCE=os.path.join(os.path.dirname(os.path.abspath(__file__)),"..","..","game","core","GameScripts.cpp")
def read_wrapper_calls(binary_path):
    out=subprocess.run(["objdump","-d","--no-show-raw-insn",binary_path],capture_output=True,text=True).stdout
    res={}; cur=None
    for l in out.splitlines():
        m=re.match(r"[0-9a-f]+ <_ZN13GenericScript\d+(\w+)_GenericEP16asIScriptGeneric(@@Base)?>:",l)
        if m: cur=m.group(1); res[cur]=[]; idx=None; continue
        if re.match(r"[0-9a-f]+ <",l): cur=None; continue
        if cur is None: continue
        m=re.search(r"movl\s+\$0x([0-9a-f]+),0x4\(%esp\)",l)
        if m: idx=int(m.group(1),16)
        m=re.search(r"(call|jmp)\s+\*0x([0-9a-f]+)\(%e..\)",l)
        if m: res[cur].append((int(m.group(2),16),idx)); idx=None
    return res
known={}
for m in re.finditer(r"SCRIPT_DEFINE_FUNC(?:_\d+)?\(\s*(\w+)\s*,\s*(\w+)\s*((?:,\s*\w+\s*)*)\)",open(SCRIPT_SOURCE).read()):
    known[m.group(2)]=(m.group(1),[a.strip() for a in m.group(3).split(",") if a.strip()])
ARGUMENT_GETTER_TYPES={28:"string",16:"float",8:"bool",12:"int"}
RETURN_SETTER_TYPES={40:"float",32:"bool",36:"int",48:"string"}
def decode_wrapper_signature(v):
    args={}; ret="void"; unk=[]
    for off,idx in v:
        if idx is not None and off in ARGUMENT_GETTER_TYPES: args[idx]=ARGUMENT_GETTER_TYPES[off]
        elif off in RETURN_SETTER_TYPES and idx is None: ret=RETURN_SETTER_TYPES[off]
        elif off in ARGUMENT_GETTER_TYPES and idx is None: args[max(args)+1 if args else 0]=ARGUMENT_GETTER_TYPES[off]+"?"
        else: unk.append((off,idx))
    return ret,[args.get(i,"??") for i in range(max(args)+1 if args else 0)],unk
if __name__=="__main__":
    out={}
    for b in ("blackplague.bin","requiem.bin"):
        r=read_wrapper_calls(GAME_BINARY_DIR+b)
        if b=="blackplague.bin":
            # calibration table
            cal=collections.defaultdict(collections.Counter)
            for n,(ret,args) in known.items():
                if n in r:
                    for off,idx in r[n]:
                        if idx is not None and idx<len(args): cal[off][args[idx]]+=1
                        elif idx is None: cal[off]["ret:"+ret]+=1
            print("calibration:",{k:dict(v) for k,v in cal.items()})
        for n,v in sorted(r.items()):
            d=decode_wrapper_signature(v)
            if n in known:
                if (d[0],d[1])!=known[n] or d[2]: print(b,"CHANGED/MISMATCH",n,"ours:",known[n],"bin:",d)
            else: out[n]=d
    for n,d in sorted(out.items()): print("NEW  %s %s(%s) %s"%(d[0],n,",".join(d[1]),d[2] or ""))
    json.dump(out,open(sys.argv[1],"w"),indent=1)
