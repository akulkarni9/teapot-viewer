#include "Texture.h"
#include <boost/unordered_map.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <iostream>

#undef max

namespace eh
{

    typedef boost::unordered_map<std::wstring, Texture*> TEXTURE_MAP;
    static  TEXTURE_MAP file_map;

    Texture::Texture(const std::wstring& sFileName):
            m_file(sFileName),
            m_resource(NULL)
    {
    }

    Ptr<Texture> Texture::createFromFile(const std::wstring& sFileName)
    {
        boost::filesystem::wpath file = sFileName;

        //if ( !boost::filesystem::exists( file ) )
        //{
        //    for (boost::filesystem::wdirectory_iterator it(file.parent_path()), end; it != end; ++it)
        //    {
        //        if ( boost::iequals( (*it).string(), file.string() ) )
        //        {
        //            file = *it;
        //            break;
        //        }
        //    }
        //}

        //if ( !boost::filesystem::exists( file ) )
        //{
        //    std::wcerr << L"Texture::createFromFile: " << file.string() << " not found!" << std::endl;
        //    return NULL;
        //}

		Texture*& tex = file_map[file.wstring()];

        if (tex)
            return tex;
        else
            return tex = new Texture( file.wstring() );
    }

    Texture::~Texture()
    {
        for (TEXTURE_MAP::iterator it = file_map.begin();
                it != file_map.end(); ++it)
        {
            if (it->second == this)
            {
                file_map.erase(it->first);
                return;
            }
        }
    }

} //end namespace
