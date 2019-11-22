/*
 * afhds3.cpp
 *
 *  Created on: 20.11.2019
 *      Author: jklim
 */

#include "afhds3.h"

afhds3 afhds3::intance = 0;

afhds3::Init(FlySkySerialPulsesData* data){
  afhds3::intance = new afhds3(data);
}

afhds3::afhds3(FlySkySerialPulsesData* data) {
  this->data = data;
}

afhds3::~afhds3() {

}

