# Skiff

Homemade lightweight zsh prompt leveraging `libgit2`

```shell
skiff: foo/bar/baz on  main [✘!?]
3s ❯

foo/bar/baz 🔒
[2] 1h3m22s ❯
```

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
