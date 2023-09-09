/*
 * SingletonFactory.h
 *
 *  Created on: Sep 8, 2023
 *      Author: tim
 */

#ifndef INC_SINGLETONFACTORY_H_
#define INC_SINGLETONFACTORY_H_

#include <map>
#include <memory>

// One approach to do this would be to use RTTI, e.g. get real time type info for the
// templated type T to use as the key for the singleton map
// Note in this case the singleton map would have the key type	std::type_index
/*
template<typename T, typename... Args>
std::shared_ptr<T> SingletonFactory::create(Args&&... args) {
	std::type_index typeIndex(typeid(T));
	if (singletons.find(typeIndex) == singletons.end()) {
		singletons[typeIndex] = std::make_shared<T>(std::forward<Args>(args)...);
	}
	return std::static_pointer_cast<T>(singletons[typeIndex]);
}

template<typename T>
std::shared_ptr<T> SingletonFactory::retrieve() {
	std::type_index typeIndex(typeid(T));
	if (singletons.find(typeIndex) == singletons.end()) {
		return nullptr;
	}
	return std::static_pointer_cast<T>(singletons[typeIndex]);
}
*/

// Since we can't use RTTI in this embedded platform, we can use this workaround to use
// the address of a static function, which is guaranteed to be unique
template <typename T>
struct Id {
    static void Addressed() {};
};

class SingletonFactory {
public:
	template<typename StorageType, typename CreationType, typename... Args>
	std::shared_ptr<StorageType> create(Args&&... args) {
		auto key = &Id<StorageType>::Addressed;
		if (singletons.find(key) == singletons.end()) {
			singletons[key] = std::make_shared<CreationType>(std::forward<Args>(args)...);
		}
		return std::static_pointer_cast<StorageType>(singletons[key]);
	}

	template<typename StorageType, typename... Args>
	std::shared_ptr<StorageType> create(Args&&... args) {
		return create<StorageType, StorageType>(std::forward<Args>(args)...);
	}

    template<typename StorageType>
    std::shared_ptr<StorageType> retrieve() {
    	auto key = &Id<StorageType>::Addressed;
    	if (singletons.find(key) == singletons.end()) {
    		return nullptr;
    	}
    	return std::static_pointer_cast<StorageType>(singletons[key]);
    }

private:
    static std::map<void (*)(), std::shared_ptr<void>> singletons;
};

#endif
