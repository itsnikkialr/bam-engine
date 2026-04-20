#include "Component.h"

bool Component::isEnabled() {
    if (!componentRef) return false;
    return (*componentRef)["enabled"];
}
