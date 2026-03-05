# Security

## Reporting Vulnerabilities

If you discover a security vulnerability, please report it privately via email rather than opening a public issue.

## API Key Security

This project requires an xAI API key to function. **Never commit API keys to the repository.**

### Secure Key Handling

1. **App Users**: Configure your API key through the Pebble app settings page on your phone
2. **Developers**: Use environment variables for any scripts that need API access:
   ```bash
   XAI_API_KEY=xai-your-key-here ./your-script.sh
   ```

### Pre-commit Hook

This repository includes a pre-commit hook that scans for common secret patterns before each commit. If you see a "COMMIT BLOCKED" message, remove the detected secret before committing.

To bypass in case of false positives:
```bash
git commit --no-verify
```

## Post-Incident Recovery

### For Repository Maintainers

After a git history rewrite (e.g., to remove leaked credentials):

1. **Force push the rewritten history**:
   ```bash
   git push --force-with-lease origin main
   ```

2. **Rotate the leaked credential** immediately in the xAI dashboard at [x.ai/api](https://x.ai/api)

3. **Notify collaborators** that they need to reset their local repositories:
   ```bash
   # Option A: Fresh clone (recommended)
   rm -rf grebble
   git clone https://github.com/yourusername/grebble.git

   # Option B: Reset existing clone
   cd grebble
   git fetch origin
   git reset --hard origin/main
   ```

4. **Update any CI/CD secrets** if the leaked key was used in automated pipelines

### For Collaborators

If you're notified that the repository history has been rewritten:

1. **Do not push your local branch** - it contains the old history
2. **Fresh clone** is the safest option:
   ```bash
   git clone https://github.com/yourusername/grebble.git
   ```
3. Re-apply any local uncommitted changes manually
