# Add the following to platform.ini if you need to wipe the ESP flash:
# extra_script = extra.py
# ERASE_FLASH_NOW = 1

from SCons.Script import DefaultEnvironment
env = DefaultEnvironment()


myflags=[
        "-vv",
        "-cd", "$UPLOAD_RESETMETHOD",
        "-cb", "$UPLOAD_SPEED",
        "-cp", "$UPLOAD_PORT",
        "-cf", "$SOURCE"
    ]

if str(ARGUMENTS.get("ERASE_FLASH_NOW")) == "1": myflags.append("-ce")

env.Replace(
    MYUPLOADERFLAGS=myflags,
    UPLOADCMD='$UPLOADER $MYUPLOADERFLAGS'
)
