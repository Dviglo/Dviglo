// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include <dviglo/core/process_utils.h> // GetArguments()

#include "network.h"

void ParseNetworkArguments()
{
    const Vector<String>& arguments = GetArguments();

    for (i32 i = 0; i < arguments.Size(); ++i)
    {
        String argument = arguments[i].ToLower();

        if (argument[0] == '-')
        {
            argument = argument.Substring(1);

            if (argument == "server")
            {
                runServer = true;
                runClient = false;
            }
            else if (argument == "address")
            {
                runClient = true;
                runServer = false;
                serverAddress = arguments[i + 1];
                ++i;
            }
            else if (argument == "username")
            {
                userName = arguments[i + 1];
                ++i;
            }
            else if (argument == "nobgm")
            {
                nobgm = true;
            }
        }
    }
}
