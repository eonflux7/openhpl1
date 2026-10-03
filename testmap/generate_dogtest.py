#!/usr/bin/env python3
"""Dog behaviour arena. Original entities, generated geometry and nav nodes."""
# GNU GPL v3 or later. See COPYING.
from pathlib import Path
import sys
import xml.etree.ElementTree as ET
from generate import Map, node, fmt, write_collada

# Block letters baked into wall geometry: no fonts or image tools needed.
FONT = {
 'A':['01110','10001','10001','11111','10001','10001','10001'],
 'C':['01111','10000','10000','10000','10000','10000','01111'],
 'D':['11110','10001','10001','10001','10001','10001','11110'],
 'E':['11111','10000','10000','11110','10000','10000','11111'],
 'F':['11111','10000','10000','11110','10000','10000','10000'],
 'H':['10001','10001','10001','11111','10001','10001','10001'],
 'I':['11111','00100','00100','00100','00100','00100','11111'],
 'K':['10001','10010','10100','11000','10100','10010','10001'],
 'L':['10000','10000','10000','10000','10000','10000','11111'],
 'N':['10001','11001','11001','10101','10011','10011','10001'],
 'O':['01110','10001','10001','10001','10001','10001','01110'],
 'P':['11110','10001','10001','11110','10000','10000','10000'],
 'R':['11110','10001','10001','11110','10100','10010','10001'],
 'S':['01111','10000','10000','01110','00001','00001','11110'],
 'T':['11111','00100','00100','00100','00100','00100','00100'],
 'U':['10001','10001','10001','10001','10001','10001','01110'],
 'W':['10001','10001','10001','10101','10101','10101','01010'],
 '2':['01110','10001','00001','00110','01000','10000','11111'],
 ' ':['00000']*7,
}


def sign(m, text, cx, z, size=.13):
    x0 = cx - len(text)*6*size/2
    for i, char in enumerate(text):
        for row, bits in enumerate(FONT[char]):
            for col, bit in enumerate(bits):
                if bit == '1':
                    x, y = x0+(i*6+col)*size, 3.7-row*size
                    m.box((x,y,z),(x+size*.86,y+size*.86,z+.02),
                          'testmap_mark', '_nocollide_noshadow')


def build(out):
    out.mkdir(parents=True, exist_ok=True)
    m, solids = Map(), []
    root = ET.Element('DogTestMap')

    def wall(x0,z0,x1,z1,height=4.5):
        m.box((x0,0,z0),(x1,height,z1),'testmap_wall')
        solids.append((x0,z0,x1,z1))

    m.box((-32,-.5,-30),(32,0,22),'testmap_floor')
    wall(-32,-30,-31.6,22); wall(31.6,-30,32,22)
    wall(-32,-30,32,-29.6); wall(-32,21.6,32,22)
    for x in (-11,11):
        wall(x-.2,-30,x+.2,-7); wall(x-.2,-1,x+.2,22)
    # A wide central corridor, with doorless entrances to all six bays.
    for z in (-7,-1):
        for lo,hi in ((-32,-24),(-20,-2),(2,20),(24,32)):
            wall(lo,z-.2,hi,z+.2)
    sign(m,'F2 TEST CONTROLS',0,-6.75,.10)
    for x in (-22,0,22):
        m.light('halllight'+str(x).replace('-','n'),(x,3,-4),14)

    definitions = [
        ('patrol','Patrol and turning',-22,-18,(-5,5),(0,8),
         'Watch the rectangular patrol for gait, cornering and pauses. Turn off perception to watch without a chase.'),
        ('hide','Sight and search',0,-18,(-6,-8),(-6,7),
         'Let the Dog see you down the left lane, then move behind the central cover. Compare its remembered target and search.'),
        ('sound','Sound investigation',22,-18,(-5,-7),(5,7),
         'Stay behind cover. Play lure A or B to test hearing and investigation using real game sound triggers.'),
        ('props','Obstacle recovery',-22,10,(0,-7),(0,7),
         'Reveal your position to start a pursuit. The heavy crate blocks the narrow route; the right opening offers a detour. The small crate remains pushable.'),
        ('door','Door breaking',0,10,(0,-7),(0,7),
         'Reveal your position to seed a chase behind the locked original door. Watch approach, breaking, recovery and attack timing.'),
        ('attack','Attack and getting up',22,10,(0,-5),(0,6),
         'Approach standing or crouched to test the original jumping attack. Knock down tests the real ragdoll/getting-up states. Reset revives a fresh Dog.'),
    ]
    for name,label,cx,cz,dog,player,instructions in definitions:
        pos = lambda v,y=.05: (cx+v[0],y,cz+v[1])
        m.start(name,pos(player),0)
        m.start(name+'dog',pos(dog),0 if name=='patrol' else 180)
        m.light(name+'light',(cx,3,cz),19)
        sign(m,name.upper(),cx,cz-10.5 if cz<0 else -.75)
        # Spawn/observation marks; purely visual.
        for v in (dog,player):
            x,y,z=pos(v, .005)
            m.box((x-.65,y,z-.65),(x+.65,y+.008,z+.65),'testmap_mark','_nocollide_noshadow')
        station=ET.SubElement(root,'Station',Name=name,Label=label,Instructions=instructions)
        if name=='patrol':
            for i,v in enumerate(((-5,-7),(5,-7),(5,5),(-5,5))):
                n='patrol'+str(i)
                m.nodes.append(node('_node_ground_'+n,pos(v)))
                ET.SubElement(station,'Patrol',Node=n)
        if name in ('hide','sound'):
            wall(cx-3,cz-2,cx+3,cz+2)
            if name=='hide': wall(cx+3,cz+4,cx+7,cz+4.4)
        if name=='sound':
            for label,v in (('A',(-5,1)),('B',(5,-5))):
                ET.SubElement(station,'Lure',Position=fmt(pos(v,1)),Label=label)
                x,_,z=pos(v)
                m.box((x-.4,.015,z-.4),(x+.4,.025,z+.4),'testmap_mark','_nocollide_noshadow')
        if name=='props':
            wall(cx-10.8,cz-.2,cx-.95,cz+.2)
            wall(cx+.95,cz-.2,cx+5.5,cz+.2)
            for n,file,v,y in (('heavycrate','iron_mine_crate.ent',(0,0),.65),
                               ('smallcrate','modern_mine_woodcrate_small.ent',(-3,-5),.4)):
                ET.SubElement(station,'Prop',Name=n,File=file,Position=fmt(pos(v,y)))
        if name=='door':
            wall(cx-10.8,cz-.2,cx-.8,cz+.2)
            wall(cx+.8,cz-.2,cx+10.8,cz+.2)
            ET.SubElement(station,'Prop',Name='testdoor',File='door_oldwood.ent',Position=fmt(pos((0,0),1.06)),Locked='true')
        if name=='attack':
            # Optional low step, outside the direct attack lane.
            m.box((cx+4,0,cz+1),(cx+8,.3,cz+4),'testmap_prop')
            solids.append((cx+4,cz+1,cx+8,cz+4))
    # Author a navigable graph, with body-width clearance around static walls.
    for ix in range(41):
        x=-30+ix*1.5
        for iz in range(33):
            z=-28+iz*1.5
            if any(x0-.6<=x<=x1+.6 and z0-.6<=z<=z1+.6 for x0,z0,x1,z1 in solids):
                continue
            m.nodes.append(node(f'_node_ground_grid{ix}x{iz}',(x,.05,z)))
    # One initial original Dog. The controller resets it per selected station.
    m.ref('character_dog','dogtestdog',(-27,.05,-13))
    write_collada(out/'dogtest.dae',m)
    ET.indent(root)
    ET.ElementTree(root).write(out/'dogtest.cfg',encoding='utf-8',xml_declaration=True)


if __name__=='__main__':
    build(Path(sys.argv[1]))
