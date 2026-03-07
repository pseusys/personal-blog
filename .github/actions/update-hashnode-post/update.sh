#!/usr/bin/env bash
# Update a single Hashnode post from a local markdown file.
# Skips silently if no post with a matching slug exists.
#
# Required environment variables:
#   FILE           - path to the markdown file
#   HASHNODE_PAT   - Hashnode Personal Access Token
#   HASHNODE_HOST  - publication host (e.g. yourblog.hashnode.dev)

set -euo pipefail

: "${FILE:?FILE is required}"
: "${HASHNODE_PAT:?HASHNODE_PAT is required}"
: "${HASHNODE_HOST:?HASHNODE_HOST is required}"

slug=$(basename "$FILE" .md)

# Query Hashnode for an existing post with this slug
response=$(curl -s -X POST https://gql.hashnode.com \
  -H "Content-Type: application/json" \
  -H "Authorization: $HASHNODE_PAT" \
  -d "$(jq -n \
    --arg host "$HASHNODE_HOST" \
    --arg slug "$slug" \
    '{
      query: "query($host:String!,$slug:String!){publication(host:$host){post(slug:$slug){id title}}}",
      variables: { host: $host, slug: $slug }
    }')")

post_id=$(echo "$response" | jq -r '.data.publication.post.id // empty')

if [ -z "$post_id" ]; then
  echo "No existing post with slug '$slug' — skipping."
  exit 0
fi

echo "Found post $post_id, updating..."

# Read markdown; split first H1 line as title, rest as body
content=$(cat "$FILE")
title=$(echo "$content" | head -n1 | sed 's/^#\+ *//')
body=$(echo "$content" | tail -n +2)

# Update the post
update_response=$(curl -s -X POST https://gql.hashnode.com \
  -H "Content-Type: application/json" \
  -H "Authorization: $HASHNODE_PAT" \
  -d "$(jq -n \
    --arg id "$post_id" \
    --arg title "$title" \
    --arg md "$body" \
    '{
      query: "mutation($input:UpdatePostInput!){updatePost(input:$input){post{id title url}}}",
      variables: { input: { id: $id, title: $title, contentMarkdown: $md } }
    }')")

errors=$(echo "$update_response" | jq -r '.errors // empty')
if [ -n "$errors" ] && [ "$errors" != "null" ]; then
  echo "::error::Failed to update post '$slug': $errors"
  exit 1
fi

url=$(echo "$update_response" | jq -r '.data.updatePost.post.url')
echo "Updated: $url"
