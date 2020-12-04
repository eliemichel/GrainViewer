/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#pragma once

#include <string>

/**
 * Environement variables are used for substitution of patterns like $BASEFILE in scene files
 * They are not os' env variables, rather a local definition of it.
 * TODO: This name might be misleading, rename into sth else than "environment"?
 * (This is a singleton class)
 */
struct EnvironmentVariables {
public:
    // static API
    static EnvironmentVariables& GetInstance() noexcept;
    static std::string Eval(const std::string value) { return GetInstance().eval(value); }

    EnvironmentVariables& operator=(const EnvironmentVariables&) = delete;
    EnvironmentVariables(const EnvironmentVariables&) = delete;

private:
    EnvironmentVariables() {}
    std::string eval(const std::string value) const;

public:
	std::string baseFile;
};
