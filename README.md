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
  actions/
    update-hashnode-post/       # Composite action: sync a post to Hashnode
  workflows/
    update-hashnode.yml         # Auto-update Hashnode on push to main
```

## Posts

| Post                                                                               | Source                                                                       |
|------------------------------------------------------------------------------------|------------------------------------------------------------------------------|
| [A custom NetworkManager VPN plugin 101](posts/misc/network-manager-vpn-plugin.md) | [ExampleVpnPlugin](sources/misc/network-manager-vpn-plugin/ExampleVpnPlugin) |

## Hashnode sync

A GitHub Action automatically updates existing Hashnode articles whenever the corresponding Markdown file changes on `main`. New posts are created manually on Hashnode; the action only updates posts that already exist (matched by slug).

**Required setup:**

- Secret `HASHNODE_PAT` — Hashnode Personal Access Token
- Variable `HASHNODE_HOST` — publication host (e.g. `pseusys.hashnode.dev`)

## License

Blog content and source code are provided as-is for educational purposes.
