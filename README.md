# personal-blog

Source repository for my blog at [pseusys.hashnode.dev](https://pseusys.hashnode.dev).

Blog posts live in `posts/` as Markdown files, and accompanying source code lives in `sources/` under a matching path.

## Structure

```text
posts/                          # Blog post articles (Markdown)
  misc/
    network-manager-vpn-plugin.md

sources/                        # Companion code for posts
  misc/
    network-manager-vpn-plugin/
      ExampleVpnPlugin/         # Full example NM VPN plugin (C / Meson)
      Containerfile             # Build & verify with podman

.github/
  workflows/
    lint.yml                    # Lint Markdown and spelling on push / PR
```

## Posts

| Post | Link | Source |
| --- | --- | --- |
| [A custom NetworkManager VPN plugin 101](posts/misc/network-manager-vpn-plugin.md) | [pseusys.hashnode.dev](https://pseusys.hashnode.dev/network-manager-vpn-plugin) | [ExampleVpnPlugin](sources/misc/network-manager-vpn-plugin/ExampleVpnPlugin) |

## License

Blog content and source code are provided as-is for educational purposes.
