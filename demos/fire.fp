!!ARBfp1.0

TEMP temp,spec;

#X = temp
#Y = dichte
#Z = beleuchtung
#W = 0

TEX temp, fragment.texcoord[0], texture[0], 3D;
TEX spec, temp, texture[1], 3D;

MOV result.color, spec;

END
