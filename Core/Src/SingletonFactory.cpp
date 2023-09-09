/*
 * SingletonFactory.cpp
 *
 *  Created on: Sep 8, 2023
 *      Author: tim
 */

#include <SingletonFactory.h>

// global definition of the static singleton map, required for static member variables
std::map<void (*)(), std::shared_ptr<void>> SingletonFactory::singletons;
