/*
Universal Decompiler (uvudec)
Copyright 2008 John McMaster
JohnDMcMaster@gmail.com
Licensed under terms of the three clause BSD license, see LICENSE for details
*/

/*
Binary functions as opposed those for processing config directives
(the config directive functions actually came first)
The naming in this file should admittedly be cleaned up

Ex, add 3 can be represented as

UVDBinaryFunction
	raw data recovered from analysis
	uvd
	UVDBinaryFunctionShared *m_shared
		function name: add3
		m_representations
			UVDBinaryFunctionInstance
				symbol: add3_quick
					or for an unknown function...
					symbol: uvudec__candela_PLTL_1__0FA6
				code:
					inc
					inc
					inc
			UVDBinaryFunctionInstance
				symbol: add3_short
				code:
					add 3
*/

#ifndef UVD_FUNCTION_H
#define UVD_FUNCTION_H

#include "uvd_types.h"
#include "uvd_compiler.h"
#include "uvd_data.h"
#include "uvd_version.h"
#include "uvd_relocation.h"
#include <string>
#include <vector>

/*
In practical terms, a function is loaded from a config archive by the following:
-Text file (*.func) describing what the function is called and other common information
-Entries for each representation
	-*.func file will have reference to binary (*.bin)

So at a minimum we need/should have:
-A .func file
-A .bin file to describe the function

Ex for archive SDCC_CRT.tar.bz2

printf.func
	# Common info
	NAME=printf
	DESC=Print a formatted string.
	
	# Implementation entry
	IMPL=printf__version_1_2__optimized.bin
	# Missing version IDs means "all"
	# Min version is inclusive
	VER_MIN=1
	# Max version is exclusive
	VER_MAX=2
printf__version_1_2__optimized.bin
	 


*/

//A specific implementation of function recorded from analysis 
class UVDElf;
class UVDBinaryFunctionInstance
{
public:
	UVDBinaryFunctionInstance();
	~UVDBinaryFunctionInstance();
	uv_err_t init();
	static uv_err_t getUVDBinaryFunctionInstance(UVDBinaryFunctionInstance **out);
	
	//Gets default hash
	//Will compute it if needed
	uv_err_t getHash(std::string &hash);
	//On the relocatable version
	uv_err_t getRelocatableHash(std::string &hash);

	//Get binary representations of relocatable and raw forms
	uv_err_t getRawDataBinary(UVDData **data);
	uv_err_t getRelocatableDataBinary(UVDData **data);

	//Get an ELF file relocatable
	//It is callee responsibilty to free
	uv_err_t toUVDElf(UVDElf **out);
	//Factory function to create from ELF file
	//It is callee responsibilty to free
	static uv_err_t getFromUVDElf(const UVDElf *in, UVDBinaryFunctionInstance **out);

	void setData(UVDData *data);
	UVDData *getData();

private:
	//Force hash compute
	uv_err_t computeHash();
	//Relocatable version
	uv_err_t computeRelocatableHash();
		
public:
	//Giving code for Language, compiler needed (used) to produce binary 
	UVDCompiler *m_compiler;
	//Version ranges that will produce this
	UVDSupportedVersions m_versions;
	//Project specific options needed to generate code (ex: -O3)
	UVDCompilerOptions *m_compilerOptions;
private:
	//Binary version stored in DB
	UVDData *m_data;
public:
	//The relocations for this function
	//Stuff like where g_debug is used
	//Also stored is a relocatable version of the function
	UVDRelocatableData *m_relocatableData;

	//UV_DISASM_LANG
	//Programming Language code representation is in (assembly, C, etc)
	//Assembly may sound useless, but many MCU functions are well commented assembly
	int m_language;
	//Language specific code representation
	std::string m_code;
	//Description of where this code came from (ie a specific product)
	std::string m_origin;
	//Misc notes
	std::string m_notes;
	//Hash based distribution allows quicker compare and avoids copyright issues
	std::string m_MD5;
	std::string m_relocatableMD5;
	/*
	The real name may not be known, but the symbol can be arbitrarily defined to a value to link
	For unknown symbols, this is reccomended to be the value that came from the original program
	
	Note that while the real function name is shared between various implementations,
	the symbol name is specific to a particular implementation
	*/
	std::string m_symbolName; 
};

/*
As would be queried or written to analysis DB
Represents a group of representations of a function
*/
class UVDBinaryFunctionShared
{
public:
	UVDBinaryFunctionShared();

public:
	//The real name as called in a program
	//If the real name is unknown, this should be empty
	std::string m_name;
	//Human readable description of what the function does ("converts a character to upper case")
	//For unknown functions, this may be blank or a description of how it was generated
	std::string m_description;
	//Assembly/C/fortran, varients
	std::vector<UVDBinaryFunctionInstance *> m_representations;
};

//A function as found in an executable (binary) as opposed to a config function
//Intended for current analysis
class UVD;
class UVDBinaryFunction
{
public:
	UVDBinaryFunction();

public:
	//Local binary representation of function
	//May be different than DB version for small techincal reasons (nop padding?)
	UVDData *m_data;
	//Analysis engine generated by
	UVD *m_uvd;
	//The generated function info
	//m_shared should have a single m_representations
	//maybe later we link into a db could have multiple?
	UVDBinaryFunctionShared *m_shared;
};

#endif //UVD_FUNCTION_H
