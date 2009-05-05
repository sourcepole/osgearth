/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
 * Copyright 2008-2009 Pelican Ventures, Inc.
 * http://osgearth.org
 *
 * osgEarth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <osgEarth/GeoData>
#include <osgEarth/ImageUtils>
#include <osgEarth/Mercator>
#include <osg/Notify>
#include <gdal_priv.h>
#include <gdalwarper.h>
#include <ogr_spatialref.h>


using namespace osgEarth;


GeoExtent GeoExtent::INVALID = GeoExtent();


GeoExtent::GeoExtent()
{
    //NOP - invalid
}

GeoExtent::GeoExtent(const SpatialReference* srs,
                     double xmin, double ymin, double xmax, double ymax) :
_srs( srs ),
_xmin(xmin),_ymin(ymin),_xmax(xmax),_ymax(ymax)
{
    //NOP
}

GeoExtent::GeoExtent( const GeoExtent& rhs ) :
_srs( rhs._srs ),
_xmin( rhs._xmin ), _ymin( rhs._ymin ), _xmax( rhs._xmax ), _ymax( rhs._ymax )
{
    //NOP
}

bool
GeoExtent::operator == ( const GeoExtent& rhs ) const
{
    if ( !isValid() && !rhs.isValid() )
        return true;

    else return
        isValid() && rhs.isValid() &&
        _xmin == rhs._xmin &&
        _ymin == rhs._ymin &&
        _xmax == rhs._xmax &&
        _ymax == rhs._ymax &&
        _srs.valid() && rhs._srs.valid() &&
        _srs->isEquivalentTo( rhs._srs.get() );
}

bool
GeoExtent::operator != ( const GeoExtent& rhs ) const
{
    return !( *this == rhs );
}

bool
GeoExtent::isValid() const {
    return _srs.valid() &&
           _xmin < _xmax &&
           _ymin < _ymax;
}

const SpatialReference*
GeoExtent::getSRS() const {
    return _srs.get(); 
}

double
GeoExtent::xMin() const {
    return _xmin;
}

double
GeoExtent::yMin() const {
    return _ymin;
}

double
GeoExtent::xMax() const {
    return _xmax; 
}

double
GeoExtent::yMax() const {
    return _ymax; 
}

double
GeoExtent::width() const {
    return _xmax - _xmin;
}

double
GeoExtent::height() const {
    return _ymax - _ymin;
}

GeoExtent
GeoExtent::transform( const SpatialReference* to_srs ) const 
{       
    if ( isValid() && to_srs )
    {
        /*double to_xmin, to_ymin, to_xmax, to_ymax;
        int err = 0;
        err += _srs->transform( _xmin, _ymin, to_srs, to_xmin, to_ymin )? 0 : 1;
        err += _srs->transform( _xmax, _ymax, to_srs, to_xmax, to_ymax )? 0 : 1;
        */

        double ll_x, ll_y;
        double ul_x, ul_y;
        double ur_x, ur_y;
        double lr_x, lr_y;
        int err = 0;
        err += _srs->transform(_xmin, _ymin, to_srs, ll_x, ll_y) ? 0: 1;
        err += _srs->transform(_xmin, _ymax, to_srs, ul_x, ul_y) ? 0: 1;
        err += _srs->transform(_xmax, _ymax, to_srs, ur_x, ur_y) ? 0: 1;
        err += _srs->transform(_xmax, _ymin, to_srs, lr_x, lr_y) ? 0: 1;

        double to_xmin = osg::minimum(ll_x, osg::minimum(ul_x, osg::minimum(ur_x, lr_x) ) );
        double to_ymin = osg::minimum(ll_y, osg::minimum(ul_y, osg::minimum(ur_y, lr_y) ) );
        double to_xmax = osg::maximum(ll_x, osg::maximum(ul_x, osg::maximum(ur_x, lr_x) ) );
        double to_ymax = osg::maximum(ll_y, osg::maximum(ul_y, osg::maximum(ur_y, lr_y) ) );

        osg::notify(osg::INFO) << "Transformed extent " 
            << to_xmin << ", " << to_ymin << "  to " << to_xmax << ", " << to_ymax << std::endl;



        if ( err > 0 )
        {
            osg::notify(osg::WARN)
                << "[osgEarth] Warning, failed to transform an extent from "
                << _srs->getName() << " to "
                << to_srs->getName() << std::endl;
        }
        else
        {
            return GeoExtent( to_srs, to_xmin, to_ymin, to_xmax, to_ymax );
        }
    }
    return GeoExtent(); // invalid
}

void
GeoExtent::getBounds(double &xmin, double &ymin, double &xmax, double &ymax) const
{
    xmin = _xmin;
    ymin = _ymin;
    xmax = _xmax;
    ymax = _ymax;
}

bool
GeoExtent::contains(const SpatialReference* srs, double x, double y)
{
    double local_x, local_y;
    if (srs->isEquivalentTo(_srs.get()))
    {
        //No need to transform
        local_x = x;
        local_y = y;
    }
    else
    {
        srs->transform(x, y, _srs.get(), local_x, local_y);
        //osgEarth::Mercator::latLongToMeters(y, x, local_x, local_y);        
    }

    return (local_x >= _xmin && local_x <= _xmax && local_y >= _ymin && local_y <= _ymax);
}

/***************************************************************************/


GeoImage::GeoImage(osg::Image* image, const GeoExtent& extent ) :
_image(image),
_extent(extent)
{
    //NOP
}

osg::Image*
GeoImage::getImage() const {
    return _image.get();
}

const SpatialReference*
GeoImage::getSRS() const {
    return _extent.getSRS();
}

const GeoExtent&
GeoImage::getExtent() const {
    return _extent;
}

GeoImage*
GeoImage::crop( double xmin, double ymin, double xmax, double ymax ) const
{
    double destXMin = xmin;
    double destYMin = ymin;
    double destXMax = xmax;
    double destYMax = ymax;

    osg::Image* new_image = ImageUtils::cropImage(
        _image.get(),
        _extent.xMin(), _extent.yMin(), _extent.xMax(), _extent.yMax(),
        destXMin, destYMin, destXMax, destYMax );

    //The destination extents may be different than the input extents due to not being able to crop along pixel boundaries.
    return new_image?
        new GeoImage( new_image, GeoExtent( _extent.getSRS(), destXMin, destYMin, destXMax, destYMax ) ) :
        NULL;
}

osg::Image* createImageFromDataset(GDALDataset* ds)
{
    //Allocate the image
    osg::Image *image = new osg::Image;
    image->allocateImage(ds->GetRasterXSize(), ds->GetRasterYSize(), 1, GL_RGBA, GL_UNSIGNED_BYTE);

    ds->RasterIO(GF_Read, 0, 0, image->s(), image->t(), (void*)image->data(), image->s(), image->t(), GDT_Byte, 4, NULL, 4, 4 * image->s(), 1);
    ds->FlushCache();

    image->flipVertical();

    return image;
}

GDALDataset* createMemDS(int width, int height, double minX, double minY, double maxX, double maxY, const std::string &projection)
{
    //Get the MEM driver
    GDALDriver* memDriver = (GDALDriver*)GDALGetDriverByName("MEM");
    if (!memDriver)
    {
        osg::notify(osg::NOTICE) << "Could not get MEM driver" << std::endl;
    }

    //Create the in memory dataset.
    GDALDataset* ds = memDriver->Create("", width, height, 4, GDT_Byte, 0);

    //Initialize the color interpretation
    ds->GetRasterBand(1)->SetColorInterpretation(GCI_RedBand);
    ds->GetRasterBand(2)->SetColorInterpretation(GCI_GreenBand);
    ds->GetRasterBand(3)->SetColorInterpretation(GCI_BlueBand);
    ds->GetRasterBand(4)->SetColorInterpretation(GCI_AlphaBand);

    //Initialize the geotransform
    double geotransform[6];
    double x_units_per_pixel = (maxX - minX) / (double)width;
    double y_units_per_pixel = (maxY - minY) / (double)height;
    geotransform[0] = minX;
    geotransform[1] = x_units_per_pixel;
    geotransform[2] = 0;
    geotransform[3] = maxY;
    geotransform[4] = 0;
    geotransform[5] = -y_units_per_pixel;
    ds->SetGeoTransform(geotransform);
    ds->SetProjection(projection.c_str());

    return ds;
}

GDALDataset* createDataSetFromImage(const osg::Image* image, double minX, double minY, double maxX, double maxY, const std::string &projection)
{
    //Clone the incoming image
    osg::ref_ptr<osg::Image> clonedImage = new osg::Image(*image);

    //Flip the image
    clonedImage->flipVertical();  

    GDALDataset* srcDS = createMemDS(image->s(), image->t(), minX, minY, maxX, maxY, projection);

    //Write the image data into the memory dataset
    //If the image is already RGBA, just read all 4 bands in one call
    if (image->getPixelFormat() == GL_RGBA)
    {
        srcDS->RasterIO(GF_Write, 0, 0, clonedImage->s(), clonedImage->t(), (void*)clonedImage->data(), clonedImage->s(), clonedImage->t(), GDT_Byte, 4, NULL, 4, 4 * image->s(), 1);
    }
    else if (image->getPixelFormat() == GL_RGB)
    {    
        //osg::notify(osg::NOTICE) << "Reprojecting RGB " << std::endl;
        //Read the read, green and blue bands
        srcDS->RasterIO(GF_Write, 0, 0, clonedImage->s(), clonedImage->t(), (void*)clonedImage->data(), clonedImage->s(), clonedImage->t(), GDT_Byte, 3, NULL, 3, 3 * image->s(), 1);

        //Initialize the alpha values to 255.
        unsigned char *alpha = new unsigned char[clonedImage->s() * clonedImage->t()];
        memset(alpha, 255, clonedImage->s() * clonedImage->t());

        GDALRasterBand* alphaBand = srcDS->GetRasterBand(4);
        alphaBand->RasterIO(GF_Write, 0, 0, clonedImage->s(), clonedImage->t(), alpha, clonedImage->s(),clonedImage->t(), GDT_Byte, 0, 0);

        delete[] alpha;
    }
    srcDS->FlushCache();

    return srcDS;
}

osg::Image* reprojectImage(osg::Image* srcImage, const std::string srcWKT, double srcMinX, double srcMinY, double srcMaxX, double srcMaxY,
                           const std::string destWKT, double destMinX, double destMinY, double destMaxX, double destMaxY)
{
    GDALAllRegister();

    //Create a dataset from the source image
    GDALDataset* srcDS = createDataSetFromImage(srcImage, srcMinX, srcMinY, srcMaxX, srcMaxY, srcWKT);


    void* transformer = GDALCreateGenImgProjTransformer(srcDS, NULL, NULL, destWKT.c_str(), 1, 0, 0);

    double outgeotransform[6];
    double extents[4];
    int width,height;
    GDALSuggestedWarpOutput2(srcDS,
                             GDALGenImgProjTransform, transformer,
                             outgeotransform,
                             &width,
                             &height,
                             extents,
                             0);

   
    GDALDataset* destDS = createMemDS(width, height, destMinX, destMinY, destMaxX, destMaxY, destWKT);

    GDALReprojectImage(srcDS, NULL,
                       destDS, NULL,
                       //GDALResampleAlg::GRA_NearestNeighbour,
                       GRA_Bilinear,
                       0,0,0,0,0);                    

    osg::Image* result = createImageFromDataset(destDS);
    
    delete srcDS;
    delete destDS;
    
    GDALDestroyGenImgProjTransformer(transformer);

    return result;
}    

osg::Image* manualReproject(const osg::Image* image, const GeoExtent& src_extent, const GeoExtent& dest_extent)
{
    unsigned int width = 256;
    unsigned int height = 256;

    osg::Image *result = new osg::Image();
    result->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);

    double dx = (dest_extent.xMax() - dest_extent.xMin()) / (double)width;
    double dy = (dest_extent.yMax() - dest_extent.yMin()) / (double)height;

    for (unsigned int c = 0; c < width; ++c)
    {
        double dest_x = dest_extent.xMin() + (double)c * dx;
        for (unsigned int r = 0; r < height; ++r)
        {
            double dest_y = dest_extent.yMin() + (double)r * dy;

            double src_x, src_y;

            //Convert the requested sample point to the SRS of the source image
            dest_extent.getSRS()->transform(dest_x, dest_y, src_extent.getSRS(), src_x, src_y);

            //Find the pixel in the source image that would correspond to that location
            double px = (((src_x - src_extent.xMin()) / (src_extent.xMax() - src_extent.xMin())) * (double)(image->s()-1));
            double py = (((src_y - src_extent.yMin()) / (src_extent.yMax() - src_extent.yMin())) * (double)(image->t()-1));

            int px_i = (int)px;
            int py_i = (int)py;

            osg::Vec4 color(0,0,0,0);

            int rowMin = osg::maximum((int)floor(py), 0);
            int rowMax = osg::maximum(osg::minimum((int)ceil(py), (int)(image->t()-1)), 0);
            int colMin = osg::maximum((int)floor(px), 0);
            int colMax = osg::maximum(osg::minimum((int)ceil(px), (int)(image->s()-1)), 0);

            if (rowMin > rowMax) rowMin = rowMax;
            if (colMin > colMax) colMin = colMax;

            osg::Vec4 urColor = image->getColor(colMax, rowMax);
            osg::Vec4 llColor = image->getColor(colMin, rowMin);
            osg::Vec4 ulColor = image->getColor(colMin, rowMax);
            osg::Vec4 lrColor = image->getColor(colMax, rowMin);

            /*Average Interpolation*/
            /*double x_rem = px - (int)px;
            double y_rem = py - (int)py;

            double w00 = (1.0 - y_rem) * (1.0 - x_rem);
            double w01 = (1.0 - y_rem) * x_rem;
            double w10 = y_rem * (1.0 - x_rem);
            double w11 = y_rem * x_rem;
            double wsum = w00 + w01 + w10 + w11;
            wsum = 1.0/wsum;

            color.r() = (w00 * llColor.r() + w01 * lrColor.r() + w10 * ulColor.r() + w11 * urColor.r()) * wsum;
            color.g() = (w00 * llColor.g() + w01 * lrColor.g() + w10 * ulColor.g() + w11 * urColor.g()) * wsum;
            color.b() = (w00 * llColor.b() + w01 * lrColor.b() + w10 * ulColor.b() + w11 * urColor.b()) * wsum;
            color.a() = (w00 * llColor.a() + w01 * lrColor.a() + w10 * ulColor.a() + w11 * urColor.a()) * wsum;*/

            /*Nearest Neighbor Interpolation*/
            if (px_i >= 0 && px_i < image->s() &&
                py_i >= 0 && py_i < image->t())
            {
                //osg::notify(osg::NOTICE) << "Sampling pixel " << px << "," << py << std::endl;
                color = image->getColor(px_i, py_i);
            }
            else
            {
                osg::notify(osg::NOTICE) << "Pixel out of range " << px_i << "," << py_i << "  image is " << image->s() << "x" << image->t() << std::endl;
            }

            /*Bilinear interpolation*/
            //Check for exact value
            /*if ((colMax == colMin) && (rowMax == rowMin))
            {
                //osg::notify(osg::NOTICE) << "Exact value" << std::endl;
                color = image->getColor(px_i, py_i);
            }
            else if (colMax == colMin)
            {
                //osg::notify(osg::NOTICE) << "Vertically" << std::endl;
                //Linear interpolate vertically
                for (unsigned int i = 0; i < 4; ++i)
                {
                    color[i] = ((float)rowMax - py) * llColor[i] + (py - (float)rowMin) * ulColor[i];
                }
            }
            else if (rowMax == rowMin)
            {
                //osg::notify(osg::NOTICE) << "Horizontally" << std::endl;
                //Linear interpolate horizontally
                for (unsigned int i = 0; i < 4; ++i)
                {
                    color[i] = ((float)colMax - px) * llColor[i] + (px - (float)colMin) * lrColor[i];
                }
            }
            else
            {
                //osg::notify(osg::NOTICE) << "Bilinear" << std::endl;
                //Bilinear interpolate
                for (unsigned int i = 0; i < 4; ++i)
                {
                float r1 = ((float)colMax - px) * llColor[i] + (px - (float)colMin) * lrColor[i];
                float r2 = ((float)colMax - px) * ulColor[i] + (px - (float)colMin) * urColor[i];

                //osg::notify(osg::INFO) << "r1, r2 = " << r1 << " , " << r2 << std::endl;
                color[i] = ((float)rowMax -py) * r1 + (py - (float)rowMin) * r2;
                }
            }*/

            result->data(c, r)[0] = (unsigned char)(color.r() * 255);
            result->data(c, r)[1] = (unsigned char)(color.g() * 255);
            result->data(c, r)[2] = (unsigned char)(color.b() * 255);
            result->data(c, r)[3] = (unsigned char)(color.a() * 255);
        }
    }
    return result;
}



GeoImage*
GeoImage::reproject(const SpatialReference* to_srs, const GeoExtent* to_extent) const
{  
    GeoExtent destExtent;
    if (to_extent)
    {
        destExtent = *to_extent;
    }
    else
    {
         destExtent = getExtent().transform(to_srs);    
    }

    //osg::notify(osg::NOTICE) << "Reprojecting image " << std::endl;
   
    /*osg::Image* resultImage = reprojectImage(getImage(),
                                       getSRS()->getWKT(),
                                       getExtent().xMin(), getExtent().yMin(), getExtent().xMax(), getExtent().yMax(),
                                       to_srs->getWKT(),
                                       destExtent.xMin(), destExtent.yMin(), destExtent.xMax(), destExtent.yMax());*/
    osg::Image* resultImage = manualReproject(getImage(), getExtent(), *to_extent);
    return new GeoImage(resultImage, destExtent);
}


/***************************************************************************/
GeoHeightField::GeoHeightField(osg::HeightField* heightField, const GeoExtent& extent)
{
    _extent = extent;
    _heightField = heightField;

    double minx, miny, maxx, maxy;
    _extent.getBounds(minx, miny, maxx, maxy);

    _heightField->setOrigin( osg::Vec3d( minx, miny, 0.0 ) );
    _heightField->setXInterval( (maxx - minx)/(double)(_heightField->getNumColumns()-1) );
    _heightField->setYInterval( (maxy - miny)/(double)(_heightField->getNumRows()-1) );
    _heightField->setBorderWidth( 0 );
}

bool GeoHeightField::getElevation(const osgEarth::SpatialReference *srs, double x, double y, ElevationInterpolation interp, float &elevation)
{
    double local_x, local_y;
    if (_extent.getSRS()->isEquivalentTo(srs))
    {
        //No need to transform
        local_x = x;
        local_y = y;
    }
    else
    {
        if (!srs->transform(x, y, _extent.getSRS(), local_x, local_y)) return false;
    }

    if (_extent.contains(_extent.getSRS(), local_x, local_y))
    {
        elevation = HeightFieldUtils::getHeightAtLocation(_heightField.get(), local_x, local_y, interp);
        return true;
    }
    else
    {
        elevation = 0.0f;
        return false;
    }
}

const GeoExtent&
GeoHeightField::getGeoExtent() const
{
    return _extent;
}

const osg::HeightField*
GeoHeightField::getHeightField() const
{
    return _heightField.get();
}