Import("env")

env.AddCustomTarget(
    name="j2build",
    dependencies=None,
    actions=[
        "platformio run -j 2",
    ],
    title="j2build",
    description="Build (max 2 jobs)"
)

env.AddCustomTarget(
    name="j2upload",
    dependencies=None,
    actions=[
        "platformio run -j 2 --target upload",
    ],
    title="j2upload",
    description="Upload (max 2 jobs)"
)

env.AddCustomTarget(
    name="j3build",
    dependencies=None,
    actions=[
        "platformio run -j 3",
    ],
    title="j3build",
    description="Build (max 3 jobs)"
)

env.AddCustomTarget(
    name="j3upload",
    dependencies=None,
    actions=[
        "platformio run -j 3 --target upload",
    ],
    title="j3upload",
    description="Upload (max 3 jobs)"
)
