# Skiff

Homemade lightweight zsh prompt leveraging `libgit2`

![Preview](https://github.com/xiaoshihou514/skiff/assets/108414369/7454d84c-8a3c-4c7f-9e4b-0859b4db1396)

## Installation

You need `libgit2-devel` and `gcc`

```zsh
git clone https://github.com/xiaoshihou514/skiff
cd skiff && ./install.sh
```

Add the following to your zsh config:

```zsh
eval "$(skiff init)"
```

Inspired by [starship](https://github.com/starship/starship/)
