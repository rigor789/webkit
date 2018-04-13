"use strict";

const assert = require('assert');
require('../tools/js/v3-models.js');
const MockModels = require('./resources/mock-v3-models.js').MockModels;
const MockRemoteAPI = require('../unit-tests/resources/mock-remote-api.js').MockRemoteAPI;

function createPatch()
{
    return new UploadedFile(453, {'createdAt': new Date('2017-05-01T19:16:53Z'), 'filename': 'patch.dat', 'extension': '.dat', 'author': 'some user',
        size: 534637, sha256: '169463c8125e07c577110fe144ecd63942eb9472d438fc0014f474245e5df8a1'});
}

function createRoot()
{
    return new UploadedFile(456, {'createdAt': new Date('2017-05-01T21:03:27Z'), 'filename': 'root.dat', 'extension': '.dat', 'author': 'some user',
        size: 16452234, sha256: '03eed7a8494ab8794c44b7d4308e55448fc56f4d6c175809ba968f78f656d58d'});
}

function customCommitSetWithoutOwnedCommit()
{
    const customCommitSet = new CustomCommitSet;
    customCommitSet.setRevisionForRepository(MockModels.osx, '10.11.4 15E65');
    customCommitSet.setRevisionForRepository(MockModels.webkit, '200805');
    return customCommitSet;
}

function customCommitSetWithOwnedCommit()
{
    const customCommitSet = new CustomCommitSet;
    customCommitSet.setRevisionForRepository(MockModels.osx, '10.11.4 15E65');
    customCommitSet.setRevisionForRepository(MockModels.ownerRepository, 'OwnerRepository-r0');
    customCommitSet.setRevisionForRepository(MockModels.ownedRepository, 'OwnedRepository-r0', null, 'OwnerRepository-r0');
    return customCommitSet;
}

function customCommitSetWithPatch()
{
    const customCommitSet = new CustomCommitSet;
    const patch = createPatch();
    customCommitSet.setRevisionForRepository(MockModels.osx, '10.11.4 15E65');
    customCommitSet.setRevisionForRepository(MockModels.webkit, '200805', patch);
    return customCommitSet;
}

function customCommitSetWithOwnedRepositoryHasSameNameAsNotOwnedRepository()
{
    const customCommitSet = new CustomCommitSet;
    customCommitSet.setRevisionForRepository(MockModels.osx, '10.11.4 15E65');
    customCommitSet.setRevisionForRepository(MockModels.webkit, '200805');
    customCommitSet.setRevisionForRepository(MockModels.ownedWebkit, 'owned-200805', null, '10.11.4 15E65');
    return customCommitSet;
}

function ownerCommit()
{
    return new CommitLog(5, {
        repository: MockModels.ownerRepository,
        revision: 'owner-commit-0',
        ownsCommits: true,
        time: null,
    });
}

function partialOwnerCommit()
{
    return new CommitLog(5, {
        repository: MockModels.ownerRepository,
        revision: 'owner-commit-0',
        ownsCommits: null,
        time: +(new Date('2016-05-13T00:55:57.841344Z')),
    });
}

function ownedCommit()
{
    return new CommitLog(6, {
        repository: MockModels.ownedRepository,
        revision: 'owned-commit-0',
        ownsCommits: null,
        time: 1456932774000
    });
}

function webkitCommit()
{
    return new CommitLog(2017, {
        repository: MockModels.webkit,
        revision: 'webkit-commit-0',
        ownsCommits: false,
        time: 1456932773000
    });
}

describe('IntermediateCommitSet', () => {
    MockRemoteAPI.inject();
    MockModels.inject();

    describe('setCommitForRepository', () => {
        it('should allow set commit for owner repository', () => {
            const commitSet = new IntermediateCommitSet(new CommitSet);
            const commit = ownerCommit();
            commitSet.setCommitForRepository(MockModels.ownerRepository, commit);
            assert.equal(commit, commitSet.commitForRepository(MockModels.ownerRepository));
        });

        it('should allow set commit for owned repository', () => {
            const commitSet = new IntermediateCommitSet(new CommitSet);
            const commit = ownerCommit();

            const fetchingPromise = commit.fetchOwnedCommits();
            const requests = MockRemoteAPI.requests;
            assert.equal(requests.length, 1);
            assert.equal(requests[0].url, '../api/commits/111/owned-commits?owner-revision=owner-commit-0');
            assert.equal(requests[0].method, 'GET');

            requests[0].resolve({commits: [{
                id: 233,
                repository: MockModels.ownedRepository.id(),
                revision: '6f8b0dbbda95a440503b88db1dd03dad3a7b07fb',
                time: +(new Date('2016-05-13T00:55:57.841344Z')),
            }]});

            return fetchingPromise.then(() => {
                const ownedCommit = commit.ownedCommits()[0];
                commitSet.setCommitForRepository(MockModels.ownerRepository, commit);
                commitSet.setCommitForRepository(MockModels.ownedRepository, ownedCommit);
                assert.equal(commit, commitSet.commitForRepository(MockModels.ownerRepository));
                assert.equal(ownedCommit, commitSet.commitForRepository(MockModels.ownedRepository));
                assert.deepEqual(commitSet.repositories(), [MockModels.ownerRepository, MockModels.ownedRepository]);
            });
        });
    });

    describe('fetchCommitLogs', () => {

        it('should fetch CommitLog object with owned commits information',  () => {
            const commit = partialOwnerCommit();
            assert.equal(commit.ownsCommits(), null);
            const owned = ownedCommit();

            const commitSet = CommitSet.ensureSingleton('53246456', {revisionItems: [{commit}, {commit: owned, ownerCommit: commit}]});
            const intermediateCommitSet =new IntermediateCommitSet(commitSet);
            const fetchingPromise = intermediateCommitSet.fetchCommitLogs();

            const requests = MockRemoteAPI.requests;
            assert.equal(requests.length, 2);
            assert.equal(requests[0].url, '/api/commits/111/owner-commit-0');
            assert.equal(requests[0].method, 'GET');
            assert.equal(requests[1].url, '/api/commits/112/owned-commit-0');
            assert.equal(requests[1].method, 'GET');

            requests[0].resolve({commits: [{
                id: 5,
                repository: MockModels.ownerRepository,
                revision: 'owner-commit-0',
                ownsCommits: true,
                time: +(new Date('2016-05-13T00:55:57.841344Z')),
            }]});
            requests[1].resolve({commits: [{
                id: 6,
                repository: MockModels.ownedRepository,
                revision: 'owned-commit-0',
                ownsCommits: false,
                time: 1456932774000,
            }]});

            return MockRemoteAPI.waitForRequest().then(() => {
                assert.equal(requests.length, 3);
                assert.equal(requests[2].url, '../api/commits/111/owned-commits?owner-revision=owner-commit-0');
                assert.equal(requests[2].method, 'GET');

                requests[2].resolve({commits: [{
                    id: 6,
                    repository: MockModels.ownedRepository.id(),
                    revision: 'owned-commit-0',
                    ownsCommits: false,
                    time: 1456932774000,
                }]});
                return fetchingPromise;
            }).then(() => {
                assert(commit.ownsCommits());
                assert.equal(commit.ownedCommits().length, 1);
                assert.equal(commit.ownedCommits()[0], owned);
                assert.equal(owned.ownerCommit(), commit);
                assert.equal(owned.repository(), MockModels.ownedRepository);
                assert.equal(intermediateCommitSet.commitForRepository(MockModels.ownedRepository), owned);
                assert.equal(intermediateCommitSet.ownerCommitForRepository(MockModels.ownedRepository), commit);
                assert.deepEqual(intermediateCommitSet.repositories(), [MockModels.ownerRepository, MockModels.ownedRepository]);
            });
        });
    });

    describe('updateRevisionForOwnerRepository', () => {

        it('should update CommitSet based on the latest invocation', () => {
            const commitSet = new IntermediateCommitSet(new CommitSet);
            const firstUpdatePromise = commitSet.updateRevisionForOwnerRepository(MockModels.webkit, 'webkit-commit-0');
            const secondUpdatePromise = commitSet.updateRevisionForOwnerRepository(MockModels.webkit, 'webkit-commit-1');
            const requests = MockRemoteAPI.requests;

            assert(requests.length, 2);
            assert.equal(requests[0].url, '/api/commits/11/webkit-commit-0');
            assert.equal(requests[0].method, 'GET');
            assert.equal(requests[1].url, '/api/commits/11/webkit-commit-1');
            assert.equal(requests[1].method, 'GET');

            requests[1].resolve({commits: [{
                id: 2018,
                repository: MockModels.webkit.id(),
                revision: 'webkit-commit-1',
                ownsCommits: false,
                time: 1456932774000,
            }]});

            let commit = null;
            return secondUpdatePromise.then(() => {
                commit = commitSet.commitForRepository(MockModels.webkit);

                requests[0].resolve({commits: [{
                    id: 2017,
                    repository: MockModels.webkit.id(),
                    revision: 'webkit-commit-0',
                    ownsCommits: false,
                    time: 1456932773000,
                }]});

                assert.equal(commit.revision(), 'webkit-commit-1');
                assert.equal(commit.id(), 2018);

                return firstUpdatePromise;
            }).then(() => {
                const currentCommit = commitSet.commitForRepository(MockModels.webkit);
                assert.equal(commit, currentCommit);
            });
        });

    });

    describe('removeCommitForRepository', () => {
        it('should remove owned commits when owner commit is removed', () => {
            const commitSet = new IntermediateCommitSet(new CommitSet);
            const commit = ownerCommit();

            const fetchingPromise = commit.fetchOwnedCommits();
            const requests = MockRemoteAPI.requests;
            assert.equal(requests.length, 1);
            assert.equal(requests[0].url, '../api/commits/111/owned-commits?owner-revision=owner-commit-0');
            assert.equal(requests[0].method, 'GET');

            requests[0].resolve({commits: [{
                id: 233,
                repository: MockModels.ownedRepository.id(),
                revision: '6f8b0dbbda95a440503b88db1dd03dad3a7b07fb',
                ownsCommits: true
            }]});

            return fetchingPromise.then(() => {
                commitSet.setCommitForRepository(MockModels.ownerRepository, commit);
                commitSet.setCommitForRepository(MockModels.ownedRepository, commit.ownedCommits()[0]);
                commitSet.removeCommitForRepository(MockModels.ownerRepository);
                assert.deepEqual(commitSet.repositories(), []);
            });
        });

        it('should not remove owner commits when owned commit is removed', () => {
            const commitSet = new IntermediateCommitSet(new CommitSet);
            const commit = ownerCommit();

            const fetchingPromise = commit.fetchOwnedCommits();
            const requests = MockRemoteAPI.requests;
            assert.equal(requests.length, 1);
            assert.equal(requests[0].url, '../api/commits/111/owned-commits?owner-revision=owner-commit-0');
            assert.equal(requests[0].method, 'GET');

            requests[0].resolve({commits: [{
                id: 233,
                repository: MockModels.ownedRepository.id(),
                revision: '6f8b0dbbda95a440503b88db1dd03dad3a7b07fb',
                time: +(new Date('2016-05-13T00:55:57.841344Z')),
            }]});

            return fetchingPromise.then(() => {
                commitSet.setCommitForRepository(MockModels.ownerRepository, commit);
                commitSet.setCommitForRepository(MockModels.ownedRepository, commit.ownedCommits()[0]);
                commitSet.removeCommitForRepository(MockModels.ownedRepository);
                assert.deepEqual(commitSet.repositories(), [MockModels.ownerRepository]);
            });
        });

        it('should not update commit set for repository if removeCommitForRepository called before updateRevisionForOwnerRepository finishes', () => {
            const commitSet = new IntermediateCommitSet(new CommitSet);
            const commit = webkitCommit();
            commitSet.setCommitForRepository(MockModels.webkit, commit);
            const updatePromise = commitSet.updateRevisionForOwnerRepository(MockModels.webkit, 'webkit-commit-1');

            commitSet.removeCommitForRepository(MockModels.webkit);

            const requests = MockRemoteAPI.requests;
            assert.equal(requests[0].url, '/api/commits/11/webkit-commit-1');
            assert.equal(requests[0].method, 'GET');

            requests[0].resolve({commits: [{
                id: 2018,
                repository: MockModels.webkit.id(),
                revision: 'webkit-commit-1',
                ownsCommits: false,
                time: 1456932774000,
            }]});

            return updatePromise.then(() => {
                assert.deepEqual(commitSet.repositories(), []);
                assert(!commitSet.commitForRepository(MockModels.webkit));
            });
        });

    });

});

describe('CustomCommitSet', () => {
    MockModels.inject();

    describe('Test custom commit set without owned commit', () => {
        it('should have right revision for a given repository', () => {
            const commitSet = customCommitSetWithoutOwnedCommit();
            assert.equal(commitSet.revisionForRepository(MockModels.osx), '10.11.4 15E65');
            assert.equal(commitSet.revisionForRepository(MockModels.webkit), '200805');
        });

        it('should have no patch for any repository', () => {
            const commitSet = customCommitSetWithoutOwnedCommit();
            assert.equal(commitSet.patchForRepository(MockModels.osx), null);
            assert.equal(commitSet.patchForRepository(MockModels.webkit), null);
        });

        it('should have no owner revision for a given repository', () => {
            const commitSet = customCommitSetWithoutOwnedCommit();
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.osx), null);
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.webkit), null);
        });

        it('should return all repositories in it', () => {
            const commitSet = customCommitSetWithoutOwnedCommit();
            assert.deepEqual(commitSet.repositories(), [MockModels.osx, MockModels.webkit]);
        });

        it('should return only top level repositories', () => {
            const commitSet = customCommitSetWithoutOwnedCommit();
            assert.deepEqual(commitSet.topLevelRepositories(), [MockModels.osx, MockModels.webkit]);
        });
    });

    describe('Test custom commit set with owned commit', () => {
        it('should have right revision for a given repository', () => {
            const commitSet = customCommitSetWithOwnedCommit();
            assert.equal(commitSet.revisionForRepository(MockModels.osx), '10.11.4 15E65');
            assert.equal(commitSet.revisionForRepository(MockModels.ownerRepository), 'OwnerRepository-r0');
            assert.equal(commitSet.revisionForRepository(MockModels.ownedRepository), 'OwnedRepository-r0');
        });

        it('should have no patch for any repository', () => {
            const commitSet = customCommitSetWithOwnedCommit();
            assert.equal(commitSet.patchForRepository(MockModels.osx), null);
            assert.equal(commitSet.patchForRepository(MockModels.ownerRepository), null);
            assert.equal(commitSet.patchForRepository(MockModels.ownedRepository), null);
        });

        it('should have right owner revision for an owned repository', () => {
            const commitSet = customCommitSetWithOwnedCommit();
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.osx), null);
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.ownerRepository), null);
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.ownedRepository), 'OwnerRepository-r0');
        });

        it('should return all repositories in it', () => {
            const commitSet = customCommitSetWithOwnedCommit();
            assert.deepEqual(commitSet.repositories(), [MockModels.osx, MockModels.ownerRepository, MockModels.ownedRepository]);
        });

        it('should return only top level repositories', () => {
            const commitSet = customCommitSetWithOwnedCommit();
            assert.deepEqual(commitSet.topLevelRepositories(), [MockModels.osx, MockModels.ownerRepository]);
        });
    });

    describe('Test custom commit set with patch', () => {
        it('should have right revision for a given repository', () => {
            const commitSet = customCommitSetWithPatch();
            assert.equal(commitSet.revisionForRepository(MockModels.osx), '10.11.4 15E65');
            assert.equal(commitSet.revisionForRepository(MockModels.webkit), '200805');
        });

        it('should have a patch for a repository with patch specified', () => {
            const commitSet = customCommitSetWithPatch();
            assert.equal(commitSet.patchForRepository(MockModels.osx), null);
            assert.deepEqual(commitSet.patchForRepository(MockModels.webkit), createPatch());
        });

        it('should have no owner revision for a given repository', () => {
            const commitSet = customCommitSetWithPatch();
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.osx), null);
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.webkit), null);
        });

        it('should return all repositories in it', () => {
            const commitSet = customCommitSetWithPatch();
            assert.deepEqual(commitSet.repositories(), [MockModels.osx, MockModels.webkit]);
        });

        it('should return only top level repositories', () => {
            const commitSet = customCommitSetWithPatch();
            assert.deepEqual(commitSet.topLevelRepositories(), [MockModels.osx, MockModels.webkit]);
        });
    });

    describe('Test custom commit set with owned repository has same name as non-owned repository',  () => {
        it('should have right revision for a given repository', () => {
            const commitSet = customCommitSetWithOwnedRepositoryHasSameNameAsNotOwnedRepository();
            assert.equal(commitSet.revisionForRepository(MockModels.osx), '10.11.4 15E65');
            assert.equal(commitSet.revisionForRepository(MockModels.webkit), '200805');
            assert.equal(commitSet.revisionForRepository(MockModels.ownedWebkit), 'owned-200805');
        });

        it('should have no patch for any repository', () => {
            const commitSet = customCommitSetWithOwnedRepositoryHasSameNameAsNotOwnedRepository();
            assert.equal(commitSet.patchForRepository(MockModels.osx), null);
            assert.equal(commitSet.patchForRepository(MockModels.webkit), null);
            assert.equal(commitSet.patchForRepository(MockModels.ownedWebkit), null);
        });

        it('should have right owner revision for an owned repository', () => {
            const commitSet = customCommitSetWithOwnedRepositoryHasSameNameAsNotOwnedRepository();
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.osx), null);
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.webkit), null);
            assert.equal(commitSet.ownerRevisionForRepository(MockModels.ownedWebkit), '10.11.4 15E65');
        });

        it('should return all repositories in it', () => {
            const commitSet = customCommitSetWithOwnedRepositoryHasSameNameAsNotOwnedRepository();
            assert.deepEqual(commitSet.repositories(), [MockModels.osx, MockModels.webkit, MockModels.ownedWebkit]);
        });

        it('should return only top level repositories', () => {
            const commitSet = customCommitSetWithOwnedRepositoryHasSameNameAsNotOwnedRepository();
            assert.deepEqual(commitSet.topLevelRepositories(), [MockModels.osx, MockModels.webkit]);
        });
    });

    describe('Test custom commit set equality function', () => {
        it('should be equal to same custom commit set', () => {
            assert.deepEqual(customCommitSetWithoutOwnedCommit(), customCommitSetWithoutOwnedCommit());
            assert.deepEqual(customCommitSetWithOwnedCommit(), customCommitSetWithOwnedCommit());
            assert.deepEqual(customCommitSetWithPatch(), customCommitSetWithPatch());
            assert.deepEqual(customCommitSetWithOwnedRepositoryHasSameNameAsNotOwnedRepository(),
                customCommitSetWithOwnedRepositoryHasSameNameAsNotOwnedRepository());
        });

        it('should not be equal even if non-owned revisions are the same', () => {
            const commitSet0 = customCommitSetWithoutOwnedCommit();
            const commitSet1 = customCommitSetWithOwnedRepositoryHasSameNameAsNotOwnedRepository();
            assert.equal(commitSet0.equals(commitSet1), false);
        });
    });

    describe('Test custom commit set custom root operations', () => {
        it('should return empty custom roots if no custom root specified', () => {
            assert.deepEqual(customCommitSetWithoutOwnedCommit().customRoots(), []);
        });

        it('should return root if root is added into commit set', () => {
            const commitSet = customCommitSetWithoutOwnedCommit();
            commitSet.addCustomRoot(createRoot());
            assert.deepEqual(commitSet.customRoots(), [createRoot()]);
        });
    });
});
