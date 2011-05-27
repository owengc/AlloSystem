#include "allocore/al_Allocore.hpp"
#include "allocore/graphics/al_Shader.hpp"
using namespace al;


// point sprite vertex shader
const char * vPointSprite =
"uniform float spriteRadius;"
"void main(){"
"	gl_FrontColor = gl_Color;"
"	gl_Position = gl_Vertex;"
"}";

// point sprite fragment shader
const char * fPointSprite =
"uniform sampler2D texSampler0;"
"void main(){"
"	gl_FragColor = texture2D(texSampler0, gl_TexCoord[0].xy) * gl_Color;"
"}";

// point sprite geometry shader
const char * gPointSprite =
"#version 120\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
"uniform float spriteRadius;"
"void main(){"
	//screen-aligned axes
"	vec3 axis1 = vec3(	gl_ModelViewMatrix[0][0],"
"						gl_ModelViewMatrix[1][0],"
"						gl_ModelViewMatrix[2][0]) * spriteRadius;"
						
"	vec3 axis2 = vec3(	gl_ModelViewMatrix[0][1],"
"						gl_ModelViewMatrix[1][1],"
"						gl_ModelViewMatrix[2][1]) * spriteRadius;"

"	vec4 pxy = gl_ModelViewProjectionMatrix * vec4(-axis1 - axis2, 0.5);"
"	vec4 pXy = gl_ModelViewProjectionMatrix * vec4( axis1 - axis2, 0.5);"
"	vec4 pxY = gl_ModelViewProjectionMatrix * vec4(-axis1 + axis2, 0.5);"
"	vec4 pXY = gl_ModelViewProjectionMatrix * vec4( axis1 + axis2, 0.5);"

"	for(int i = 0; i < gl_VerticesIn; ++i){"
		// copy color
"		gl_FrontColor = gl_FrontColorIn[i];"

//"		vec3 p = gl_PositionIn[i].xyz;"
//"		gl_TexCoord[0] = vec4(1, 1, 0, 1);"
//"		gl_Position = gl_ModelViewProjectionMatrix * vec4(p + axis1 + axis2, 1.);"
//"		EmitVertex();"
//
//"		gl_TexCoord[0] = vec4(1, 0, 0, 1);"
//"		gl_Position = gl_ModelViewProjectionMatrix * vec4(p + axis1 - axis2, 1.);"
//"		EmitVertex();"
//
//"		gl_TexCoord[0] = vec4(0, 1, 0, 1);"
//"		gl_Position = gl_ModelViewProjectionMatrix * vec4(p - axis1 + axis2, 1.);"
//"		EmitVertex();"
//
//"		gl_TexCoord[0] = vec4(0, 0, 0, 1);"
//"		gl_Position = gl_ModelViewProjectionMatrix * vec4(p - axis1 - axis2, 1.);"
//"		EmitVertex();"

"		vec4 p = gl_ModelViewProjectionMatrix * vec4(gl_PositionIn[i].xyz, 0.5);"

"		gl_TexCoord[0] = vec4(0, 0, 0, 1);"
"		gl_Position =  p + pxy;"
"		EmitVertex();"

"		gl_TexCoord[0] = vec4(1, 0, 0, 1);"
"		gl_Position =  p + pXy;"
"		EmitVertex();"

"		gl_TexCoord[0] = vec4(0, 1, 0, 1);"
"		gl_Position =  p + pxY;"
"		EmitVertex();"

"		gl_TexCoord[0] = vec4(1, 1, 0, 1);"
"		gl_Position =  p + pXY;"
"		EmitVertex();"

"		EndPrimitive();"
"	}"

//"	EndPrimitive();"	
"	gl_Position = gl_PositionIn[0];"
"}";


struct MyWindow : Window{

	MyWindow()
	:	 tex(gl, 16,16, Texture::LUMINANCE, Texture::FLOAT32)
	{}

	GraphicsGL gl;
	ShaderProgram shaderP;
	Shader shaderV, shaderF, shaderG;
	Texture tex;
	Mesh geom;
	float angle;

	bool onCreate(){

		tex.allocate();
		int Nx = tex.width();
		int Ny = tex.height();
		float * texBuf = tex.data<float>();
		
		for(int j=0; j<Ny; ++j){ float y = float(j)/(Ny-1)*2-1;
		for(int i=0; i<Nx; ++i){ float x = float(i)/(Nx-1)*2-1;
			float m = 1-al::clip(x*x + y*y);
			texBuf[j*Nx + i] = m*=m*=m;
		}}

		// create sprite positions
		int N = 32;
		geom.primitive(gl.POINTS);
		for(int k=0; k<N; ++k){ float z = float(k)/(N-1)*2-1;
		for(int j=0; j<N; ++j){ float y = float(j)/(N-1)*2-1;
		for(int i=0; i<N; ++i){ float x = float(i)/(N-1)*2-1;
			geom.vertex(x,y,z);
			geom.color(x*0.1+0.1, y*0.1+0.1, z*0.1+0.1);
		}}}

		shaderV.source(vPointSprite, Shader::VERTEX).compile().printLog();
		shaderP.attach(shaderV);

		shaderF.source(fPointSprite, Shader::FRAGMENT).compile().printLog();
		shaderP.attach(shaderF);
		
		shaderG.source(gPointSprite, Shader::GEOMETRY).compile().printLog();
		shaderP.setGeometryInputPrimitive(gl.POINTS);
		shaderP.setGeometryOutputPrimitive(gl.TRIANGLE_STRIP);
		shaderP.setGeometryOutputVertices(4);
		shaderP.attach(shaderG);

		shaderP.link().printLog();
		return true;
	}

	bool onFrame(){

		gl.clearColor(0);
		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
		gl.viewport(0,0, width(), height());
		gl.matrixMode(gl.PROJECTION);
		gl.loadMatrix(Matrix4d::perspective(45, aspect(), 0.1, 100));
		gl.matrixMode(gl.MODELVIEW);
		
		angle += 0.001;
		Matrix4d mvmat = Matrix4d::lookAt(Vec3d(0,0,-3), Vec3d(0,0,0), Vec3d(0,1,0));
		mvmat.Mat4d::rotate(angle*10, 2,0).Mat4d::rotate(angle*7, 0,1);
		gl.loadMatrix(mvmat);

		gl.depthTesting(0);
		gl.blending(true, gl.SRC_ALPHA, gl.ONE);

		shaderP.begin();
		tex.bind();

			shaderP.uniform("spriteRadius", 1./cbrt(geom.vertices().size()));
			//gl.pointSize(10); gl.antialiasing(gl.NICEST);
			gl.draw(geom);

		tex.unbind();
		shaderP.end();

//		printf("fps: %g\n", avgFps());

		return true;
	}
};

int main(){
	MyWindow win;
	win.add(new StandardWindowKeyControls);
	win.create();
	MainLoop::start();
	return 0;
}