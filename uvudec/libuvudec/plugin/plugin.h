/*
UVNet Universal Decompiler (uvudec)
Copyright 2010 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

#ifndef PLUGIN_PLUGIN_H
#define PLUGIN_PLUGIN_H

#include "uvd_arg.h"
#include "uvd_types.h"
#include "uvd_version.h"
#include <set>
#include <map>

//Entry point for a loaded plugin to register itself
#define UVD_PLUGIN_MAIN_SYMBOL					UVDPluginMain
#define UVD_PLUGIN_MAIN_SYMBOL_STRING			"UVDPluginMain"
//This isn't really for use
//#define UVD_PLUGIN_MAIN_MANGLED_SYMBOL			_Z13UVDPluginMainP9UVDConfigPP9UVDPlugin
#define UVD_PLUGIN_MAIN_MANGLED_SYMBOL_STRING	"_Z13UVDPluginMainP9UVDConfigPP9UVDPlugin"

/*
Unless otherwise specified, no functions need to be called on the parent UVDPlugin class if overriden
Don't throw exceptions, I won't catch them
If you need to do plugin specific initialization even before the plugin is activated
	Should be rare
	Do it in PluginMain()
*/
class UVDVersion;
class UVD;
class UVDConfig;
class UVDArchitecture;
class UVDData;
class UVDPlugin
{
public:
	//<name, version range>
	typedef std::pair<std::string, UVDVersionRange> PluginDependency;
	typedef std::set<PluginDependency> PluginDependencies;
	/*
	Since we want plugins to be able to process args, they are loaded very early
	They will have a uvd engine object set later as needed
	In theory, we could initialize multiple config/uvd engines this way
	but wouldn't count on it for some time
	most likely we'd just unload and reload the plugin
	*/
	typedef uv_err_t (*PluginMain)(UVDConfig *config, UVDPlugin **out);

public:
	UVDPlugin();
	virtual ~UVDPlugin();
	
	/*
	Make the plugin active
	This is the difference between producing the plugin in UVDMain which just makes it availible
	Do not assume any UVD engines are active yet
	This is done early on so can add argument structs and such
	*/
	virtual uv_err_t init(UVDConfig *config);
	//Deactivate w/e the plugin does
	virtual uv_err_t deinit(UVDConfig *config);

	/*
	Called upon UVD::init(), not UVDInit()
	was init(UVD *uvd)
	*/
	virtual uv_err_t onUVDInit(UVD *uvd);
	virtual uv_err_t onUVDDeinit(UVD *uvd);

	/*
	Note the following may be called before init()
	*/
	//Terse name to identify the plugin
	//No spaces, use [a-z][A-Z][0-9][-]
	virtual uv_err_t getName(std::string &out) = 0;
	//Human readable one liner description of what it does
	virtual uv_err_t getDescription(std::string &out) = 0;	
	//One liner contact info of who wrote it
	//Should be in form like "John McMaster <JohnDMcMaster@gmail.com>"
	//If more than one, put in comma separated list
	virtual uv_err_t getAuthor(std::string &out) = 0;	
	//Should be obvious enough
	virtual uv_err_t getVersion(UVDVersion &out) = 0;
	/*
	If we require another plugin loaded, get it
	Default is no dependencies
	
	WARNING: if you rely on symbols exposed at the library level by other plugins,
	we will not be able to cleanly load them!
	You will have to LD_PRELOAD them or something and they will not be officially supported
	*/
	virtual uv_err_t getDependencies(PluginDependencies &out);
	/*
	If we deem appropriete, load architecture
	Returns UV_ERR_NOTSUPPORTED if we can't support the data format
		such as a plugin that doesn't support architectures
	Since many of the formats will be binary, we should specify the correct plugin instead of spamming
	since it will be difficult/impossible to tell what architecture it is simply from the binary
	Note that we could support multiple archs in the plugin, we just return the best one
	*/
	virtual uv_err_t getArchitecture(UVDData *data, const std::string &architecture, UVDArchitecture **out);

	//Plugins should register through here
	uv_err_t registerArgument(const std::string &propertyForm,
			char shortForm, std::string longForm, 
			std::string helpMessage,
			uint32_t numberExpectedValues,
			UVDArgConfigHandler handler,
			bool hasDefault);
	uv_err_t registerArgument(const std::string &propertyForm,
			char shortForm, std::string longForm, 
			std::string helpMessage,
			std::string helpMessageExtra,
			uint32_t numberExpectedValues,
			UVDArgConfigHandler handler,
			bool hasDefault);

public:
	//returned by dlopen()
	void *m_hLibrary;
	/*
	This will be set upon UVD engine initialization
	Since plugins are loaded before even arguments are parsed, no UVD exists at that time

	WARNING
	It is unlikely, but possible we might in the future allow multiple UVD engines availible at once
	More likely they will have to be different application instances
	*/
	UVD *m_uvd;
};

#endif
