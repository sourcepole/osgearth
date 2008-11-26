#include <osgEarth/MapConfig>
#include <osgEarth/TileSource>
#include <osgEarth/PlateCarre>
#include <osgEarth/FileCache>

#include <osg/Notify>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <sstream>
#include <iomanip>

using namespace osgEarth;

#define PROPERTY_URL        "url"
#define PROPERTY_MAP        "map"
#define PROPERTY_LAYER      "layer"
#define PROPERTY_FORMAT     "format"
#define PROPERTY_MAP_CONFIG "map_config"

class AGSMapCacheSource : public TileSource
{
public:
    AGSMapCacheSource( const osgDB::ReaderWriter::Options* _options ) :
      options( _options ),
      map_config(0)
    {
        if ( options.valid() )
        {
            // this is the AGS virtual directory pointing to the map cache
            if ( options->getPluginData( PROPERTY_URL ) )
                url = std::string( (const char*)options->getPluginData( PROPERTY_URL ) );

            // the name of the map service cache
            if ( options->getPluginData( PROPERTY_MAP ) )
                map = std::string( (const char*)options->getPluginData( PROPERTY_MAP ) );

            // the layer, or null to use the fused "_alllayers" cache
            if ( options->getPluginData( PROPERTY_LAYER ) )
                layer = std::string( (const char*)options->getPluginData( PROPERTY_LAYER ) );

            // the image format (defaults to "png")
            // TODO: read this from the XML tile schema file
            if ( options->getPluginData( PROPERTY_FORMAT ) )
                format = std::string( (const char*)options->getPluginData( PROPERTY_FORMAT ) );


            if (options->getPluginData( PROPERTY_MAP_CONFIG ))
                map_config = (const MapConfig*)options->getPluginData( PROPERTY_MAP_CONFIG );
        }

        // validate dataset
        if ( layer.empty() ) layer = "_alllayers"; // default to the AGS "fused view"
        if ( format.empty() ) format = "png";
    }

    osg::Image* createImage( const TileKey* key )
    {
        std::stringstream buf;

        int level = key->getLevelOfDetail()-1;

        unsigned int tile_x, tile_y;
        key->getTileXY( tile_x, tile_y );
        // need to invert the y-tile index
        //tile_y = key->getMapSizeTiles()/2 - tile_y - 1;

        buf << url << "/" << map 
            << "/Layers/" << layer
            << "/L" << std::hex << std::setw(2) << std::setfill('0') << level
            << "/R" << std::hex << std::setw(8) << std::setfill('0') << tile_y
            << "/C" << std::hex << std::setw(8) << std::setfill('0') << tile_x << "." << format;

        //osg::notify(osg::NOTICE) << "Key = " << key->str() << ", URL = " << buf.str() << std::endl;

        if ( osgDB::containsServerAddress( url ) )
        {
            buf << ".curl"; // forces use of the CURL plugin to download
        }

        std::string cache_path = map_config ? map_config->getFullCachePath() : std::string("");
        bool offline = map_config ? map_config->getOfflineHint() : false;

        osgEarth::FileCache fc(cache_path);
        fc.setOffline(offline);
        return fc.readImageFile( buf.str(), options.get() );
    }

    osg::HeightField* createHeightField( const TileKey* key )
    {
        //TODO
        return NULL;
    }

private:
    osg::ref_ptr<const osgDB::ReaderWriter::Options> options;
    std::string url;
    std::string map;
    std::string layer;
    std::string format;

    const MapConfig* map_config;
};


class ReaderWriterAGSMapCache : public osgDB::ReaderWriter
{
    public:
        ReaderWriterAGSMapCache()
        {
            supportsExtension( "arcgis_map_cache", "ArcGIS Server Map Service Cache" );
        }

        virtual const char* className()
        {
            return "ArcGIS Server Map Service Cache Imagery ReaderWriter";
        }

        virtual ReadResult readObject(const std::string& file_name, const Options* options) const
        {
            return readNode( file_name, options );
        }

        virtual ReadResult readImage(const std::string& file_name, const Options* options) const
        {
            if ( osgDB::getLowerCaseFileExtension( file_name ) != "arcgis_map_cache" )
                return ReadResult::FILE_NOT_HANDLED;

            osg::ref_ptr<TileKey> key = TileKeyFactory::createFromName(
                file_name.substr( 0, file_name.find_first_of( '.' ) ) );

            osg::Image* image = NULL;

            if ( dynamic_cast<PlateCarreTileKey*>( key.get() ) )
            {
                osg::ref_ptr<AGSMapCacheSource> source = new AGSMapCacheSource( options );
                image = source->createImage( key.get() );
            }

            return image? ReadResult( image ) : ReadResult( "Unable to load ArcGIS Server map cache tile" );
        }

        virtual ReadResult readHeightField(const std::string& file_name, const Options* opt) const
        {
            return ReadResult::FILE_NOT_HANDLED;
            //NYI
        }

        virtual ReadResult readNode(const std::string& file_name, const Options* opt) const
        {
            return ReadResult::FILE_NOT_HANDLED;
        }
};

REGISTER_OSGPLUGIN(arcgis_map_cache, ReaderWriterAGSMapCache)
