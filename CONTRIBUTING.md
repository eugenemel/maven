Guidelines for developers
=========================

1.  Changes should be made in a branch and submitted via Pull Request to the
    master branch.
2.  Pull Requests should be reviewed and Travis and Appveyor builds must
    succeed before merging.
3.  Keep the `CHANGELOG.md` up to date.
4.  This project adheres to [Semantic Versioning](http://semver.org/).


Continuous Integration Builds
-----------------------------

Builds for Linux and Mac versions are run via Travis.

Builds for Windows are run via Appveyor.

Tagged versions are automatically released on GitHub releases.


Releases
--------

1.  Update `CHANGELOG.md` from `Unreleased` to `<version>`, update diff links.

2.  Merge all PRs, ensure pipelines pass. Be sure to update local repo with
    merge commit (e.g. `git checkout master; git fetch upstream master; git
        merge upstream master`)

3.  Tag with new version:
    ```
    git tag -a <version>
    git push --tags
    ```

4.  Check GitHub releases, add helpful release notes, etc.

5.  Prepare for new development (can be pushed directly to master):

    1.  Update `CHNAGELOG.md` to add `Unreleased` section, add diff link:
        ```
        [Unreleased]: https://github.com/eugenemel/maven/compare/<version>...HEAD
        ```
