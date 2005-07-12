%function testAdaptive
%TESTAdaptive  Test L-shaped problem with adaptive mesh refinement.
%   We test our diffusion equation discretization on Adaptive Mesh Refined
%   (AMR) grid. 
%   This test case is a Laplace equation on a 2D L-shaped domain, and we
%   apply adaptive mesh refinement to get increasingly higher accuracies
%   (that stagnate with the # of levels, if we do not refine all existing
%   levels as we add more levels).
%
%   See also: ADDGRIDLEVEL, ADDGRIDPATCH, TESTDISC.

% Revision history:
% 12-JUL-2005    Oren Livne    Added comments

globalParams;

fprintf('=========================================================================\n');
fprintf(' Testing adaptive mesh refinement for L-shaped Laplace problem\n');
fprintf('=========================================================================\n');

%=========================================================================
% Initialize parameters struct
%=========================================================================
param                       = [];

param.problemType           = 'Lshaped'; %'ProblemB'; %'quadratic'; %'Lshaped'; %
param.outputDir             = 'ProblemA_1Level';
param.plotResults           = 0;
param.saveResults           = 1;
param.verboseLevel          = 0;

%=========================================================================
% Create a sequence of adaptive mesh refinement levels
%=========================================================================
success                     = mkdir('.',param.outputDir);

% Create an empty grid
resolution          = [8 8]; %[4 4];                                    % Coarsest level resolution
numGlobalLevels     = 2; %3;
threshold           = 1e-5;
maxVars             = 5000;
grid.maxLevels  	= 6; %20;
grid.maxPatches  	= 100;
grid.level          = cell(grid.maxLevels,1);
grid.numLevels      = 0;
grid.domainSize     = [1.0 1.0];                                % Domain is from [0.,0.] to [1.,1.]
grid.dim            = length(grid.domainSize);
A                   = [];
b                   = [];
T                   = [];
TI                  = [];

% Prepare quantities to be saved for each refinement stage
AMR                 = cell(grid.maxLevels,1);
errNorm             = zeros(grid.maxLevels,5);
patchID             = cell(grid.maxLevels,1);

for numLevels = 1:grid.maxLevels,
    pack;
    fprintf('#### numLevels = %d ####\n',numLevels);
    
    %-------------------------------------------------------------------------
    % Set up grid (AMR levels, patches)
    %-------------------------------------------------------------------------
    if (param.verboseLevel >= 1)
        fprintf('-------------------------------------------------------------------------\n');
        fprintf(' Set up grid & system\n');
        fprintf('-------------------------------------------------------------------------\n');
    end
    tStartCPU           = cputime;
    tStartElapsed       = clock;

    if (numLevels == 1)
        [grid,k]            = addGridLevel(grid,'meshsize',grid.domainSize./resolution);
        [grid,q]            = addGridPatch(grid,k,ones(1,grid.dim),resolution,-1);     % One global patch
        patchID{numLevels}  = q;
    else
        [grid,k]            = addGridLevel(grid,'refineRatio',[2 2]);
        if (numLevels <= numGlobalLevels)
            [grid,q]       = addGridPatch(grid,k,ones(1,grid.dim),2.^(numLevels-1)*resolution,patchID{k-1});
            patchID{numLevels}  = q;
        else
            [ilower,iupper,needRefinement] = adaptiveRefinement(AMR,grid,k-1,threshold);
            if (~needRefinement)
                fprintf('No more refinement levels needed, stopping\n');
                break;
            end
            [grid,q]       = addGridPatch(grid,k,ilower,iupper,patchID{k-1});
            patchID{numLevels}  = q;
        end
    end
    if (grid.totalVars > maxVars)
        fprintf('Reached maximum allowed #vars, stopping\n');
        break;
    end
    tCPU        = cputime - tStartCPU;
    tElapsed    = etime(clock,tStartElapsed);
    if (param.verboseLevel >= 1)
        fprintf('CPU time     = %f\n',tCPU);
        fprintf('Elapsed time = %f\n',tElapsed);
        printGrid(grid);
    end
    %     % Plot grid
    %         plotGrid(grid,sprintf('%s/grid%d.eps',param.outputDir,numLevels),0,0,0,0);

    %-------------------------------------------------------------------------
    % Update the linear system with the new patches
    %-------------------------------------------------------------------------
    for q = 1:grid.level{k}.numPatches,
        [grid,A,b,T,TI]      = updateSystem(grid,k,q,A,b,T,TI);
    end
    % Plot grid
    plotGrid(grid,sprintf('%s/grid%d.eps',param.outputDir,numLevels),0,0,0,0);

    %-------------------------------------------------------------------------
    % Solve the linear system
    %-------------------------------------------------------------------------
    if (param.verboseLevel >= 1)
        fprintf('-------------------------------------------------------------------------\n');
        fprintf(' Solve the linear system\n');
        fprintf('-------------------------------------------------------------------------\n');
    end
    tStartCPU       = cputime;
    tStartElapsed   = clock;
    x               = A\b;                            % Direct solver
    u               = sparseToAMR(x,grid,TI,1);           % Translate the solution vector to patch-based
    tCPU            = cputime - tStartCPU;
    tElapsed        = etime(clock,tStartElapsed);
    if (param.verboseLevel >= 1)
        fprintf('CPU time     = %f\n',tCPU);
        fprintf('Elapsed time = %f\n',tElapsed);
    end

    %-------------------------------------------------------------------------
    % Computed exact solution vector, patch-based and compute
    % discretization error
    %-------------------------------------------------------------------------
    if (param.verboseLevel >= 1)
        fprintf('-------------------------------------------------------------------------\n');
        fprintf(' Compute exact solution, plot\n');
        fprintf('-------------------------------------------------------------------------\n');
    end
    tStartCPU        = cputime;
    tStartElapsed    = clock;

    % Plot and print discretization error at all patches
    uExact = exactSolutionAMR(grid,T,TI);
    tau = sparseToAMR(b-A*AMRToSparse(uExact,grid,T,1),grid,TI,0);
    f = sparseToAMR(b,grid,TI,0);
    fig = 0;

    % AMR grid norms
    err = cell(size(u));
    for k = 1:grid.numLevels,
        level = grid.level{k};
        for q = 1:grid.level{k}.numPatches,
            err{k}{q} = uExact{k}{q}-u{k}{q};
        end
    end
    temp    = AMRToSparse(err,grid,T,1);
    err     = sparseToAMR(temp,grid,TI,0);
    errNorm(numLevels,:) = [ ...
        numLevels ...
        normAMR(grid,err,'L2') ...
        normAMR(grid,err,'max') ...
        normAMR(grid,err,'H1') ...
        normAMR(grid,err,'H1max') ...
        ];
    fprintf('#vars = %5d  L2=%.3e  max=%.3e  H1=%.3e  H1max=%.3e\n',grid.totalVars,errNorm(numLevels,2:end));

    if (param.plotResults)
        plotResults(grid,u,uExact,tau,numCells);
    end
    tCPU        = cputime - tStartCPU;
    tElapsed    = etime(clock,tStartElapsed);
    if (param.verboseLevel >= 1)
        fprintf('CPU time     = %f\n',tCPU);
        fprintf('Elapsed time = %f\n',tElapsed);

    end
    
    % Save quantities of this refinement stage
    AMR{numLevels}.grid = grid;
    AMR{numLevels}.A = A;
    AMR{numLevels}.b = b;
    AMR{numLevels}.T = T;
    AMR{numLevels}.TI = TI;
    AMR{numLevels}.u = u;
end

if (param.saveResults)
    saveResults(errNorm);
end
