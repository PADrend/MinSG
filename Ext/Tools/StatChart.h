/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef STATCHART_H
#define STATCHART_H

#include "../../Core/Statistics.h"
#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <vector>
#include <string>

namespace Util {
class Bitmap;
}
namespace MinSG{
/**
 * Graphical chart representing Statistics events results.
 * [StatChart]
 */
class StatChart{
	public:
		struct DataRow{
			std::string description;
			Util::Color4ub color;
			float range;
			float scale;

			DataRow() : description(), color(), range(1.0f), scale(1.0f) {
			}
		};

		MINSGAPI StatChart(uint32_t width,uint32_t height,float timeRange=50.0);
		StatChart(const StatChart & other) = delete;
		StatChart(StatChart &&) = default;
		MINSGAPI ~StatChart();

		MINSGAPI uint32_t getWidth() const;
		MINSGAPI uint32_t getHeight() const;
		const Util::Reference<Util::Bitmap> & getBitmap() const {
			return bitmap;
		}

		float getTimeRange()const					{	return timeRange;	}
		void setTimeRange(float newRange)			{   timeRange=newRange;	}

		int getRowCount()const						{	return dataRows.size();	}

		void setRange(Statistics::eventType_t type,float range){
			if(dataRows.size() <= type) dataRows.resize(type+1);
			dataRows[type].range=range;
			dataRows[type].scale=getHeight()/range;
		}
		float getRange(Statistics::eventType_t type)const{
			return dataRows[type].range;
		}
		void setColor(Statistics::eventType_t type,const Util::Color4ub & color){
			if(dataRows.size() <= type) dataRows.resize(type+1);
			dataRows[type].color=color;
		}
		const Util::Color4ub & getColor(Statistics::eventType_t type)const{
			return dataRows[type].color;
		}
		void setDescription(Statistics::eventType_t type,const std::string & description){
			if(dataRows.size() <= type) dataRows.resize(type+1);
			dataRows[type].description=description;
		}
		std::string getDescription(Statistics::eventType_t type)const{
			return dataRows[type].description;
		}
		MINSGAPI void update(const Statistics & fStats);

	private:
		Util::Reference<Util::Bitmap> bitmap;

		float timeRange;
		std::vector<DataRow> dataRows;
};
}
#endif // STATCHART_H
