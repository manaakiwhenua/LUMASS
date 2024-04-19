#!/usr/bin/env python

from sys import stderr

import argparse as ap

from pythonutils.extractor import Extractor
from pythonutils.distiller import Distiller

# Author name - use script name as specified to argument parser
SCRIPT_NAME = "makeWrapperConfigFile"

# Tuple of C/C++ access specifiers
ACCESS_SPECIFIERS = ("public", "protected", "private")

if __name__ == "__main__":
    # Argument parser for command line arguments
    parser = ap.ArgumentParser(
        prog=SCRIPT_NAME,
        description="Parses wrapper information out of a C++ header file for ITK / OTB filters and stores it in a config file for further processing",
    )

    # Define arguments accepted by parser
    parser.add_argument(
        "inputHeaderFile",
        type=str,
        help="the path to the input C++ header file (supports .h header files only)",
    )
    parser.add_argument(
        "outputConfigFile",
        type=str,
        help="the path to the output config file (currently produces a .lwp file)",
    )
    parser.add_argument(
        "--componentName",
        type=str,
        help="an optional separate name for the component when it's displayed",
    )
    parser.add_argument(
        "--forwardInputUserIDs", type=str, help="optional function name"
    )
    parser.add_argument(
        "--componentIsSink",
        action="store_true",
        help="a flag to set if the wrapped component is a sink",
    )

    # Parse the command line arguments
    args = parser.parse_args()

    # Sanity check of input file - bail out early if not a C/C++ header
    if not args.inputHeaderFile.endswith(".h"):
        parser.error(
            "Invalid input file. Please specify a .h header file containing your filter declaration."
        )

    # Open input file for reading
    with open(args.inputHeaderFile, mode="r") as source:
        # Initialise dictionary of output values
        config = dict()

        # Initialise currently applicable C/C++ access specifier to unknown
        accessLevel = None

        # Initialise namespace string (may remain None if not present in input file)
        namespaceIdentifier = None

        # Value extractor utility
        extractor = Extractor()

        # Record function to forward user input IDs if given
        # TODO: there seem to be only two examples of function names, SetImageNames
        #      and SetInputNames. Investigate if this is the case and the entry could
        #      be parsed out of the header file directly instead.
        if args.forwardInputUserIDs:
            config["ForwardInputUserIDs"] = args.forwardInputUserIDs

        # Whether the component is a sink or a source/(source, sink) combination
        # needs to come from the command line
        # TODO: investigate reliable criteria for parsing this from the header instead.
        if args.componentIsSink:
            config["ComponentIsSink"] = 1
        else:
            config["ComponentIsSink"] = 0

        # Extract file name without extension
        config["FilterClassFileName"] = extractor.extractFilterClassFileName(
            args.inputHeaderFile
        )

        # Read input file one line at a time
        while line := source.readline():
            # Trim leading and trailing whitespace, turn all internal spaces into a single blank
            line = " ".join(line.split())

            if line.startswith("namespace"):
                namespaceIdentifier = extractor.extractNamespace(line)

            elif line.startswith("class"):
                # Class declaration may be over several lines, collect them
                bufferedLine = line
                if not line.endswith("{"):
                    # Keep reading - but not past EOF
                    while line := source.readline():
                        line = " ".join(line.split())
                        # Break at start of member declarations
                        if line.startswith("{"):
                            break
                        # Add more content to the buffered class declaration line
                        else:
                            bufferedLine = " ".join([bufferedLine, line])

                # Extract the name of the output wrapper class
                extractedValue = extractor.extractWrapperClassName(bufferedLine)
                config = extractor.addToConfigIfNotNone(
                    "WrapperClassName", extractedValue, config
                )

                # Extract FilterTypeDef (in combination with namespaceIdentifier)
                extractedValue = extractor.extractFilterTypeDef(
                    namespaceIdentifier, bufferedLine
                )
                config = extractor.addToConfigIfNotNone(
                    "FilterTypeDef", extractedValue, config
                )

                # Extract number of template arguments if the class is templated
                extractedValue = extractor.extractNumTemplateArgs(bufferedLine)
                config = extractor.addToConfigIfNotNone(
                    "NumTemplateArgs", extractedValue, config
                )

                # Set or extract component name for display
                if not args.componentName:
                    extractedValue = extractor.extractComponentName(bufferedLine)
                    config = extractor.addToConfigIfNotNone(
                        "ComponentName", extractedValue, config
                    )
                else:
                    config["ComponentName"] = args.componentName

            # Check for an access specifier
            elif line.startswith(ACCESS_SPECIFIERS):
                # Update the access specifier access specifier
                accessLevel = extractor.extractAccessSpecifier(line, ACCESS_SPECIFIERS)

            # Only process public blocks
            if accessLevel == "public":
                if line.startswith("AttributeTable::Pointer getRAT"):
                    config["RATGetSupport"] = 1

                # The functions we're interested in always return null
                elif line.startswith("void"):
                    # The function signature could be across more than one line
                    bufferedLine = line
                    if not line.endswith(";"):
                        # Keep reading - but not past EOF
                        while line := source.readline():
                            line = " ".join(line.split())
                            # Break at end of function signature
                            if line.endswith(";"):
                                break
                            # Add more content to the buffered function signature line
                            else:
                                bufferedLine = " ".join([bufferedLine, line])

                    # We're interested in function signatures, not member variables of type void*
                    # that may be typecast to something else later (gross but not impossible)
                    if "(" in bufferedLine and ")" in bufferedLine:
                        # Turn the function signature into a list of word-like elements
                        tokens = bufferedLine.split()

                        # The function name will be in the token that comes after the return type
                        token = tokens[1]

                        # Functions for RAT set support all have the same name
                        if token.startswith("setRAT"):
                            config["RATSetSupport"] = 1

                        # We have a candidate for an InputTypeFunc
                        else:
                            functionName = extractor.extractFunctionName(token)
                            inputType = extractor.extractInputArgumentType(bufferedLine)
                            extractedValue = extractor.extractInputTypeFunc(
                                functionName, inputType
                            )
                            config = extractor.addToConfigListIfNotNone(
                                "InputTypeFunc", extractedValue, config
                            )

            # TODO: unclear how to decide what should be captured as a Property (and how to detect it)

    # Sanity check: dictionary contents need to meet a few minimum requirements
    minimumRequirementsMet = (
        "WrapperClassName" in config
        and config["WrapperClassName"] is not None
        and "FilterClassFileName" in config
        and config["FilterClassFileName"] is not None
        and "FilterTypeDef" in config
        and config["FilterTypeDef"] is not None
    )
    if not minimumRequirementsMet:
        # Not enough information, can't proceed
        print(
            "Error processing header file: couldn't extract one of WrapperClassName, FilterClassFileName, or FilterTypeDef.\
             Are you sure your header file defines an ITK / OTB filter?",
            file=stderr,
        )
        exit(1)

    # Author, Year and FileDate come out of the environment
    config["Author"] = SCRIPT_NAME
    config["Year"] = extractor.extractYear()
    config["FileDate"] = extractor.extractFileDate()

    # write results to config file
    with open(args.outputConfigFile, mode="w") as destination:
        # Config entry line builder
        distiller = Distiller()

        # Mandatory minimum entries
        destination.writelines([distiller.distilInt("Year", config["Year"])])
        destination.writelines(
            [distiller.distilString("WrapperClassName", config["WrapperClassName"])]
        )
        destination.writelines([distiller.distilString("FileDate", config["FileDate"])])
        destination.writelines([distiller.distilString("Author", config["Author"])])
        destination.writelines(
            [
                distiller.distilString(
                    "FilterClassFileName", config["FilterClassFileName"]
                )
            ]
        )
        destination.writelines(
            [distiller.distilString("FilterTypeDef", config["FilterTypeDef"])]
        )

        # Optional int fields
        distiller.writeIntToFileIfNotNone("RATGetSupport", config, destination)
        distiller.writeIntToFileIfNotNone("RATSetSupport", config, destination)
        distiller.writeIntToFileIfNotNone("NumTemplateArgs", config, destination)
        distiller.writeIntToFileIfNotNone("ComponentIsSink", config, destination)

        # Optional string fields
        distiller.writeStringToFileIfNotNone("ComponentName", config, destination)
        distiller.writeStringToFileIfNotNone("ForwardInputUserIDs", config, destination)

        # Optional list fields
        distiller.writeListToFileIfNotNone("InputTypeFunc", config, destination)
        distiller.writeListToFileIfNotNone("Property", config, destination)
