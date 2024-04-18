from typing import Any

import datetime
import re


class Extractor:
    # ---Constructor--- #
    def __init__(self) -> None:
        self._classname = None
        self._templateParams = None

    # ---Internal utils--- #
    def _getClassName(self, line: str) -> str:
        # Sanity check
        if not line:
            return None

        # Fill in cached value if there's none yet
        if not self._classname:
            # Extract the bit before the parent class
            self._classname = line.split(":")[0]

            # Class name will be the word at the end
            self._classname.strip()
            self._classname = self._classname.split()[-1]

            # First letter should be upper case for further processing
            self._classname = self._classname[0].upper() + self._classname[1:]

        return self._classname

    def _getTemplateParameters(self, line: str) -> list[str]:
        # Sanity check - no bracket, no template parameters
        if not line or not "<" in line:
            return None

        # Populate cached field if necessary
        if not self._templateParams:
            # Template parameters are a comma-separated list between brackets
            self._templateParams = line.split("<")[-1].split(">")[0].split(",")
            self._templateParams = [t.strip() for t in self._templateParams]

        return self._templateParams

    # ---Non-dictionary functions--- #
    def extractAccessSpecifier(self, line, accessSpecifiers: list[str]) -> str:
        # Pack up the list of words of interests in a regex
        pattern = "|".join(accessSpecifiers)

        # Extract and return the first match
        accessLevel = re.match(pattern, line).group()

        return accessLevel

    def addToConfigIfNotNone(
        self, key: str, value: Any, dictionary: dict[str, Any]
    ) -> dict[str, Any]:
        if value is not None:
            dictionary[key] = value

        return dictionary

    # ---Dictionary functions--- #
    def extractNamespace(self, line: str) -> str:
        namespaceIdentifier = line.split()[1]
        return namespaceIdentifier

    def extractYear(self) -> int:
        today = datetime.date.today()

        return today.year

    def extractWrapperClassName(self, line: str) -> str:
        # Extract class name
        classname = self._getClassName(line)

        # Embed class name in wrapper name
        if classname is not None:
            return "NM{}Wrapper".format(classname)
        else:
            return None

    def extractFileDate(self) -> str:
        today = datetime.date.today()

        return str(today)

    def extractFilterClassFileName(self, filePath: str) -> str:
        # Remove file extension
        filterClassFileName = filePath.split(".")[-2]

        # Remove folder in path if present
        if "/" in filterClassFileName:
            filterClassFileName = filterClassFileName.split("/")[-1]
        elif "\\" in filterClassFileName:
            filterClassFileName = filterClassFileName.split("\\")[-1]

        # Return extracted value
        return filterClassFileName

    def extractFilterTypeDef(self, namespaceIdentifier: str, line: str) -> str:
        # Bail out early if no class name is found
        classname = self._getClassName(line)
        if classname is None:
            return None

        # Prepend the namespace identifier if there is one
        if namespaceIdentifier is not None:
            classname = "::".join([namespaceIdentifier, classname])

        # Extract the template type signature
        templateParams = self._getTemplateParameters(line)

        # Replace the template types with matching generic ones
        # TODO: this is too tailored to image-to-image filters
        #       need to be able to handle list lengths of more than 2
        #       need to be able to handle non-image template types
        if templateParams is not None and len(templateParams) == 2:
            templateParams[0] = "InImgType"
            if len(templateParams) > 1:
                templateParams[1] = "OutImgType"

            # Append the extracted template types and combine with classname
            return "".join([classname, "<", ",".join(templateParams), ">"])

        # Just need to return what we already have if there's nothing to add
        else:
            return classname

    def extractRATGetSupport(self, line: str) -> str:
        # TODO
        return None

    def extractRATSetSupport(self, line: str) -> str:
        # TODO
        return None

    def extractForwardInputUserIDs(self, line: str) -> str:
        # TODO
        return None

    def extractNumTemplateArgs(self, line: str) -> str:
        templateParams = self._getTemplateParameters(line)

        # Only return an number if the class is actually templated
        if not templateParams:
            return None
        else:
            return len(templateParams)

    def extractComponentName(self, line: str) -> str:
        # TODO: there may be a way to refine this
        return self._getClassName(line)

    # TODO: functions for Property, InputTypeFunc
