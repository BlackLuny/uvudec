/*
UVNet Universal Decompiler (uvudec)
Copyright 2010 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

#include "uvd/architecture/architecture.h"
#include "uvd/core/uvd.h"

UVDArchitecture::UVDArchitecture()
{
	m_uvd = NULL;
}

UVDArchitecture::~UVDArchitecture()
{
}

uv_err_t UVDArchitecture::init()
{
	return UV_ERR_OK;
}

uv_err_t UVDArchitecture::deinit()
{
	return UV_ERR_OK;
}

uv_err_t UVDArchitecture::readByte(UVDAddress address, uint8_t *out)
{
	//uv_assert_ret(m_uvd);
	//uv_assert_ret(m_uvd->m_runtime->m_object);
	uv_assert_err_ret(address.m_space->m_data->readData(address.m_addr, out));
	
	return UV_ERR_OK;
}

uv_err_t UVDArchitecture::parseCurrentInstruction(UVDIteratorCommon &iterCommon)
{
	//Reduce errors from stale data
	if( !iterCommon.m_instruction )
	{
		uv_assert_err_ret(getInstruction(&iterCommon.m_instruction));
		uv_assert_ret(iterCommon.m_instruction);
	}
	uv_assert_err_ret(iterCommon.m_instruction->parseCurrentInstruction(iterCommon));

	return UV_ERR_OK;
}

