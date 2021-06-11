/*
 * hui_error.h
 *
 *  Created on: 26.06.2013
 *      Author: michi
 */

#ifndef HUI_ERROR_H_
#define HUI_ERROR_H_

#include "../hui_minimal/Callback.h"

namespace hui
{


// error handling
void SetErrorFunction(const Callback &function);
void SetDefaultErrorHandler(const Callback &error_cleanup_function);
void RaiseError(const string &message);

void ShowError(const string& msg);

};

#endif /* HUI_ERROR_H_ */
