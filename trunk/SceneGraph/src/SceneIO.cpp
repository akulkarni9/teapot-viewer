/****************************************************************************
* Copyright (C) 2007-2010 by E.Heidt  http://teapot-viewer.sourceforge.net/ *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
****************************************************************************/

#include "SceneIO.h"

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <unzip.h>

namespace eh
{
	static boost::filesystem::wpath s_path;

	static void s_set_path( const boost::filesystem::wpath& _path )
	{
		s_path = _path;
		s_path.remove_filename();
	}

	static std::wstring s_abs_path(const boost::filesystem::wpath& file)
	{
		if(!file.is_complete())
			return (s_path / file).string();
		else
			return file.string();
	}

	///////////////////////////////////////////////////////////////////////////

	SceneIO::File::File(const std::wstring& file):
		m_path(file)
	{
		m_path = s_abs_path(m_path);
	}

	SceneIO::File::File(const std::string& file):
		m_path(file.begin(), file.end())
	{
		m_path = s_abs_path(m_path);
	}

	SceneIO::File::~File()
	{
	}

	const std::wstring SceneIO::File::getExtension() const
	{
		return boost::filesystem::wpath(m_path).extension();
	}

	const std::wstring SceneIO::File::getName() const
	{
		return boost::filesystem::wpath(m_path).leaf();
	}

	size_t SceneIO::File::getContent(std::auto_ptr<char>& data) const
	{
		if( boost::filesystem::exists(m_path) )
		{
			boost::filesystem::ifstream file( m_path.c_str(), std::ios_base::binary );

			if(!file.is_open())
				return 0;

			file.seekg (0, std::ios::end);
			size_t size = file.tellg();

			data.reset( new char[size] );

			file.seekg (0, std::ios::beg);
			file.read(data.get(), size);
			file.close();

			return size;
		}
		else // maybe inside zip file
		{
			boost::filesystem::wpath archive;
			boost::filesystem::wpath file;
			boost::filesystem::wpath path(m_path);
			for(boost::filesystem::wpath::const_iterator it = path.begin(); it != path.end(); ++it)
			{
				file /= *it;
				if(file.extension() == L".zip")
				{
					archive = file;
					file = L"";
				}
			}

			size_t ret = 0;

			if( unzFile zipfile = unzOpen64( std::string(archive.string().begin(), archive.string().end()).c_str()) )
			{
				for(int s = unzGoToFirstFile(zipfile); s != UNZ_END_OF_LIST_OF_FILE; s = unzGoToNextFile(zipfile))
				{
					char sName[256];
					unz_file_info64 finfo;
					unzGetCurrentFileInfo64(zipfile, &finfo, sName, 256, NULL, 0, NULL, 0);

					if( boost::iequals( std::string(sName), file.string() ) )
					{
						unzOpenCurrentFile(zipfile);

						data.reset( new char[(unsigned)finfo.uncompressed_size] );

						ret = (size_t) unzReadCurrentFile(zipfile, data.get(), (unsigned int)finfo.uncompressed_size);

						unzCloseCurrentFile(zipfile);
						break;
					}
				}

				unzClose(zipfile);

				return ret;
			}
		}

		return 0;
	}

	Ptr<Texture> SceneIO::createTexture(const std::string& text)
	{
		return createTexture(std::wstring(text.begin(), text.end()));
	}
	Ptr<Texture> SceneIO::createTexture(const std::wstring& text)
	{
		return Texture::createFromFile( s_abs_path(text) );
	}

	struct SceneIO::Impl
	{
		std::vector< IPlugIn* > m_plugins;
		std::map< std::wstring, IPlugIn* > m_ext_plugin_map;
	};

	static SceneIO::status_callback s_printStatus = NULL;

	void SceneIO::setSetStatusTextCallback(status_callback cb)
	{
		s_printStatus = cb;
	}

	void SceneIO::setStatusText(const std::wstring& text)
	{
		if(s_printStatus)
			s_printStatus(text);
	}

	SceneIO::~SceneIO()
	{
		for(size_t i = 0; i < m_pImpl->m_plugins.size(); i++)
			delete m_pImpl->m_plugins[i];

		delete m_pImpl;
	}
	SceneIO::SceneIO():m_pImpl(new Impl())
	{
		boost::filesystem::wpath path = boost::filesystem::initial_path<boost::filesystem::wpath>();

		for (boost::filesystem::wdirectory_iterator it(path), end; it != end; ++it)
		{
			if ( boost::filesystem::is_directory(it->status()) )
				continue;

			boost::filesystem::wpath file = it->leaf();

			if( !boost::iequals( file.extension(), L".dll" ) && !boost::iequals( file.extension(), L".so" ) )
				continue;

			setStatusText( std::wstring(L"Loading PlugIn: ") + file.string() + L"..." );

#if defined(_MSC_VER)

			std::wstring rpath = file.string();
			boost::algorithm::replace_all(rpath, L"/", L"\\");
			boost::algorithm::replace_all(rpath, L".dll", L"");
			if ( boost::filesystem::is_directory(rpath) )
				SetDllDirectoryW( rpath.c_str() );

			std::wstring dll = file.string();
			boost::algorithm::replace_all(dll, L"/", L"\\");
			HMODULE hModule = LoadLibraryW(dll.c_str() );
#else
            std::string fileA(file.string().begin(), file.string().end());
			void* hModule = dlopen(fileA.c_str(), RTLD_GLOBAL);
			if(!hModule)
				std::cerr << dlerror() <<  std::endl;

#endif
			typedef SceneIO::IPlugIn* (*createPlugInFunc)();

			if(hModule == NULL)
				continue;

#if defined(_MSC_VER)
			if(createPlugInFunc createPlugIn = (createPlugInFunc)GetProcAddress(hModule, "XcreatePlugIn"))
#else
			if(createPlugInFunc createPlugIn = (createPlugInFunc)dlsym(hModule, "XcreatePlugIn"))
#endif
			{
				std::wcout << file.string().c_str() << L" loaded." << std::endl;

				SceneIO::IPlugIn* pPlugIn = createPlugIn();
				m_pImpl->m_plugins.push_back( pPlugIn );

				for(Uint i = 0; i < pPlugIn->file_type_count(); i++)
				{
					std::wstring fileexts =  pPlugIn->file_exts(i);
					std::vector< std::wstring > exts;
					boost::split( exts, fileexts, boost::is_any_of(L";") );

					if(exts.size() == 0)
						exts.push_back(  pPlugIn->file_exts(i) );

					for(size_t j = 0; j < exts.size(); j++)
					{
						boost::erase_all(exts[j], "*");
						boost::algorithm::to_lower(exts[j]);

						m_pImpl->m_ext_plugin_map[exts[j]] = pPlugIn;
					}
				}
			}
			else
#if defined(_MSC_VER)
				;//std::cerr <<  "GetProcAdress failed" << std::endl;
#else
				;//std::cerr <<  dlerror() << std::endl;
#endif
			/*
			#if defined(_MSC_VER)
			FreeLibrary(hModule);
			#else
			dlclose(hModule);
			#endif
			*/
		}

		setStatusText( L"" );
	}

	std::wstring SceneIO::getAboutString() const
	{
		std::wstring about;
		about += L"-------------------------------------------\n";
		about += L"SceneIO PlugIns:\n";
		for(size_t i = 0; i < m_pImpl->m_plugins.size(); i++)
		{
			about += L"\t" + m_pImpl->m_plugins[i]->about() + L"\n";
		}
		return about;
	}

	std::wstring SceneIO::getFileWildcards(bool bLoading /*= true*/) const
	{
		std::wstring strFilter;
		if(bLoading)
		{
			strFilter += L"All known Formats|";

			for(size_t i = 0; i < m_pImpl->m_plugins.size(); i++)
			{
				for(size_t j = 0; j < m_pImpl->m_plugins[i]->file_type_count(); j++)
					if(m_pImpl->m_plugins[i]->canRead(j) )
					{
						std::wstring exts = m_pImpl->m_plugins[i]->file_exts(j);
						boost::algorithm::to_upper(exts); strFilter += exts + L";";
						boost::algorithm::to_lower(exts); strFilter += exts + L";";
					}
			}
			strFilter += L"*.zip|";
		}

		for(size_t i = 0; i < m_pImpl->m_plugins.size(); i++)
		{
			for(size_t j = 0; j < m_pImpl->m_plugins[i]->file_type_count(); j++)
				if((bLoading && m_pImpl->m_plugins[i]->canRead(j)) ||
					(!bLoading && m_pImpl->m_plugins[i]->canWrite(j)) )
				{
					std::wstring exts = m_pImpl->m_plugins[i]->file_exts(j);
					boost::algorithm::to_lower(exts);

					std::vector< std::wstring > vexts;
					boost::split( vexts,  exts, boost::is_any_of(L";") );

					if(vexts.size() == 0)
						vexts.push_back( exts );

					bool bContinue = false;
					for(size_t k = 0; k < vexts.size(); k++)
					{
						boost::erase_all(vexts[k], "*");

						if( m_pImpl->m_ext_plugin_map.find(vexts[k])->second != m_pImpl->m_plugins[i] )
							bContinue = true;
					}

					if(bContinue)
						continue;

					strFilter += m_pImpl->m_plugins[i]->file_type(j) + L" (" + exts + L")|";

					strFilter += exts + L";";
					boost::algorithm::to_lower(exts); strFilter += exts + L"|";
				}
		}

		if(bLoading)
		{
			strFilter += L"Zip Files (*.zip)|*.zip|";
			strFilter += L"All Files (*.*)|*.*||";
		}
		else
			strFilter += L"|";

		return strFilter;

	}

	bool SceneIO::execute(const std::wstring& file, Ptr<Scene> pScene, progress_callback progress, bool bLoading ) const
	{
		bool ret = false;

		progress(0.f);

		try
		{
			boost::filesystem::wpath sFile = boost::filesystem::system_complete( file );

			std::wstring ext = sFile.extension();
			boost::algorithm::to_lower(ext);

			if( m_pImpl->m_ext_plugin_map.find(ext) != m_pImpl->m_ext_plugin_map.end() )
			{
				IPlugIn* plugin = m_pImpl->m_ext_plugin_map.find(ext)->second;

				if(bLoading)
				{
		    		s_set_path( sFile );

					if(plugin->read( sFile.string(), pScene, progress) == false)
						throw -1;
					else
						ret = true;
				}
				else
				{
				   	if(plugin->write( sFile.string(), pScene, progress) == false)
						throw -1;
					else
						ret = true;
				}
			}
			else
			{
				std::string sfile(sFile.string().begin(), sFile.string().end());
				if( unzFile zipfile = unzOpen(sfile.c_str()) )
				{
					for(int s = unzGoToFirstFile(zipfile); s != UNZ_END_OF_LIST_OF_FILE; s = unzGoToNextFile(zipfile))
					{
						char sName[256];
						unz_file_info64 finfo;
						unzGetCurrentFileInfo64(zipfile, &finfo, sName, 256, NULL, 0, NULL, 0);

						std::string sName2(sName);
						boost::filesystem::wpath sFile2(sName2.begin(), sName2.end());
						std::wstring ext = sFile2.extension();
						boost::algorithm::to_lower(ext);

						if( m_pImpl->m_ext_plugin_map.find(ext) != m_pImpl->m_ext_plugin_map.end() )
						{
							IPlugIn* plugin = m_pImpl->m_ext_plugin_map.find(ext)->second;

							boost::filesystem::wpath path = sFile / sFile2;

				    		s_set_path( path );

							if(plugin->read( path.string(), pScene, progress) == false)
								ret = false;
							else
								ret = true;
						}
					}

					unzClose(zipfile);
				}
			}
		}
		catch(int e)
		{
			std::cerr << "error in " << __FUNCTION__ << " " << e << std::endl;
		}
		catch(...)
		{
			std::cerr << "Unknown Exception in " << __FUNCTION__ << std::endl;
		}

		progress(1.f);

		return ret;
	};

	static void dummy_callback(float f){}

	bool SceneIO::read(const std::wstring& sFile, Ptr<Scene> pScene, progress_callback progress) const
	{
		if(progress == NULL)
			progress = dummy_callback;

		return execute(sFile, pScene, progress, true);
	}

	bool SceneIO::write(const std::wstring& sFile, Ptr<Scene> pScene, progress_callback progress) const
	{
		if(progress == NULL)
			progress = dummy_callback;

		return execute(sFile, pScene, progress, false);
	}

	//#include <boost/thread.hpp>
	//bool FormatManager::loadAsThread(const std::wstring& sFile, Ptr<Scene> pScene) const
	//{
	//	boost::thread( boost::bind(&FormatManager::load, this, sFile, pScene ) );
	//	return false;
	//}

} //end namespace
