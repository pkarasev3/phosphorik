#include <stdio.h>
#include <string.h>

#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>

bool canRunFragmentProgram(const char* programString);
GLuint loadFragmentProgram(const char* programString);
        
// Checks for errors in the fragment program
#include <string>
#include <iostream>
using std::string;
using std::endl;
using std::cout;
bool canRunFragmentProgram(const char* programString)
{
  cout<<"testing if we can run hardware prog..."<<endl;
  string progName(programString);
  cout<<"program name: "<<progName<<endl;
	// Make sure the card supports fragment programs at all, by searching for the extension string
    const char *extensions = reinterpret_cast<const char *>( glGetString( GL_EXTENSIONS ) );
    const bool cardSupportsARB = ( strstr( extensions, "GL_ARB_fragment_program" ) != NULL );

	// If it doesn't support them, no program can run, so report the problem and return false
    if (!cardSupportsARB)
	{
        fprintf(stderr,"Card does not support the ARB_fragment_program extension\n");
        return false;
    }

	// Create a temporary ID to load the shader into
	GLuint tempID;
	glGenProgramsARB( 1, &tempID );
	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, tempID );
   
	// Get the driver to load and parse the shader
	glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB,
						GL_PROGRAM_FORMAT_ASCII_ARB,
						strlen( programString ),
						programString );

	GLint	 isUnderNativeLimits;
	glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB,
					   GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,
					   &isUnderNativeLimits );

	// If the program is over the hardware's limits, print out some information
	if (isUnderNativeLimits!=1)
	{
		// Go through the most common limits that are exceeded
		fprintf(stderr, "Fragment program is beyond hardware limits:\n");

		GLint aluInstructions, maxAluInstructions;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_ALU_INSTRUCTIONS_ARB, &aluInstructions);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB, &maxAluInstructions);
		if (aluInstructions>maxAluInstructions)
			fprintf(stderr, "Compiles to too many ALU instructions (%d, limit is %d)\n", aluInstructions, maxAluInstructions);

		GLint textureInstructions, maxTextureInstructions;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEX_INSTRUCTIONS_ARB, &textureInstructions);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB, &maxTextureInstructions);
		if (textureInstructions>maxTextureInstructions)
			fprintf(stderr, "Compiles to too many texture instructions (%d, limit is %d)\n", textureInstructions, maxTextureInstructions);

		GLint textureIndirections, maxTextureIndirections;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEX_INDIRECTIONS_ARB, &textureIndirections);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB, &maxTextureIndirections);
		if (textureIndirections>maxTextureIndirections)
			fprintf(stderr, "Compiles to too many texture indirections (%d, limit is %d)\n", textureIndirections, maxTextureIndirections);

		GLint nativeTextureIndirections, maxNativeTextureIndirections;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &nativeTextureIndirections);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &maxNativeTextureIndirections);
		if (nativeTextureIndirections>maxNativeTextureIndirections)
			fprintf(stderr, "Compiles to too many native texture indirections (%d, limit is %d)\n", nativeTextureIndirections, maxNativeTextureIndirections);

		GLint nativeAluInstructions, maxNativeAluInstructions;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &nativeAluInstructions);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &maxNativeAluInstructions);
		if (nativeAluInstructions>maxNativeAluInstructions)
			fprintf(stderr, "Compiles to too many native ALU instructions (%d, limit is %d)\n", nativeAluInstructions, maxNativeAluInstructions);
	}

 	// See if a syntax error was found
	// Often the actual line number won't be given, it will just be zero if there's an error
	// and minus one if it's ok. The error string usually includes the right line number.
	GLint errorLine;
	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorLine);
	if (errorLine!=-1)
	{
		const GLubyte* errorString = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
		fprintf(stderr,"%s",errorString);
	}
	
	glDeleteProgramsARB( 1, &tempID );

	const bool result = ((isUnderNativeLimits==1)&&(errorLine==-1));
    
    return result;
}

GLuint loadFragmentProgram(const char* programString)
{
    GLuint result;
    cout<<"testing if we can run hardware prog..."<<endl;
    string progName(programString);
    cout<<"program name: "<<progName<<endl;

    glGenProgramsARB( 1, &result );
    glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, result );
    glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB,
        GL_PROGRAM_FORMAT_ASCII_ARB,
        strlen( programString ),
        programString );

    return result;
}
