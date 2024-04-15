from typing import Any, TextIO


class Distiller:
    # ---Constructor--- #
    def __init__(self, padTo: int = 28) -> None:
        self._padTo = padTo
        self._listIndex = 0

    # ---Internal utils--- #
    def _getWhitespace(self, key: str) -> str:
        # Calculate amount of padding in string
        padding = self._padTo - len(key)
        if padding <= 0:
            padding = 1

        # Turn padding into whitespace
        whitespace = " ".replace(" ", " " * padding)

        return whitespace

    def _resetListIndex(self) -> None:
        self._listIndex = 0

    # ---File writing functions--- #
    def writeIntToFileIfNotNone(
        self, key: str, config: dict[str, Any], file: TextIO
    ) -> None:
        if key in config and config[key] is not None:
            file.writelines([self.distilInt(key, config[key])])

    def writeStringToFileIfNotNone(
        self, key: str, config: dict[str, Any], file: TextIO
    ) -> None:
        if key in config and config[key] is not None:
            file.writelines([self.distilString(key, config[key])])

    def writeListToFileIfNotNone(
        self, key: str, config: dict[str, Any], file: TextIO
    ) -> None:
        if key in config and config[key] is not None and len(config[key]) > 0:
            self._resetListIndex()
            for listEntry in config[key]:
                file.writelines([self.distilListEntry(key, listEntry)])

    # ---Key/Value formatting functions--- #
    def distilInt(self, key: str, value: int) -> str:
        line = "".join([key, self._getWhitespace(key), repr(value), "\n"])

        return line

    def distilString(self, key: str, value: str) -> str:
        line = "".join([key, self._getWhitespace(key), value, "\n"])

        return line

    def distilListEntry(self, key: str, value: str) -> str:
        # Assemble output line
        keyEntry = "".join([key, "_", str(self._listIndex)])
        line = "".join([keyEntry, self._getWhitespace(keyEntry), value])

        # Need to keep track of how many entries we're up to
        self._listIndex = self._listIndex + 1

        return line
