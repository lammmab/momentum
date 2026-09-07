#pragma once

namespace momentum::utility {
    struct Castable {
        template <typename T>
        T* To() {
            return static_cast<T*>(this);
        }

        template <typename T>
        const T* To() const {
            return static_cast<const T*>(this);
        }
    };
}
