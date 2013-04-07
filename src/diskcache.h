
/* blitwizard game engine - source code file

  Copyright (C) 2011-2013 Jonas Thiem

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#ifndef BLITWIZARD_DISKCACHE_H_
#define BLITWIZARD_DISKCACHE_H_

// This module implements a disk cache.

// Stores data in the disk cache and returns the path
// which may be used to retrieve it again.
// Please free() the path as soon as you're done with it.
char* diskcache_Store(char* data, size_t datalength);

// Retrieve a disk cache item again by path.
// On success, the callback is called with the pointer to the
// cached data and the data length.
// On failure, the callback will be passed NULL as data pointer.
// The callback will happen in another thread!
// Be sure your callback is thread-safe!
void diskcache_Retrieve(const char* path, void (*callback)(void* data,
size_t datalength, void* userdata), void* userdata);

// Delete an item from the disk cache:
void diskcache_Delete(const char* path);

#endif  // BLITWIZARD_DISKCACHE_H_

