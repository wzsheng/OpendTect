#ifndef jobman_h
#define jobman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2004
 RCS:		$Id: jobrunner.h,v 1.14 2005-03-30 11:19:22 cvsarend Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "jobinfo.h"

class IOPar;
class HostData;
class JobDescProv;
class JobIOMgr;
class StatusInfo;
class BufferStringSet;
class FilePath;


class HostNFailInfo
{
public:
    			HostNFailInfo( const HostData& hd )
			    : hostdata_(hd)
			    , nrfailures_(0)
			    , nrsucces_(0)
			    , lastsuccess_(0)
			    , starttime_(0)	{}

    const HostData&	hostdata_;
    int			nrfailures_; //!< Reset to 0 at every success
    int			nrsucces_;
    int			starttime_;  //!< Set whenever host added.
    int			lastsuccess_; //!< timestamp
};


/*!\brief Runs all jobs defined by JobDescProv. */

class JobRunner : public Executor
{
public:

    				JobRunner(JobDescProv*,const char* cmd);
				//!< JobDescProv becomes mine. Never pass null.
				~JobRunner();

    const JobDescProv*		descProv() const	{ return descprov_; }

    const ObjectSet<HostNFailInfo>& hostInfo() const	{ return hostinfo_; }
    bool			addHost(const HostData&);
    void			removeHost(int);
    void			pauseHost(int,bool);
    bool			stopAll();
    bool			hostFailed(int) const;
    bool			isPaused(int) const;
    bool			isAssigned( const JobInfo& ji ) const;

    int				nrJobs( bool failed=false ) const
    				{ return (failed ? failedjobs_ : jobinfos_)
				    	 .size(); }
    const JobInfo&		jobInfo( int idx, bool failed=false ) const
    				{ return *(failed ? failedjobs_ : jobinfos_)
				    	 [idx]; }

    int				jobsDone() const;
    int				jobsInProgress() const;
    int				jobsLeft() const
				{ return jobinfos_.size() - jobsDone(); }
    int				totalJobs() const
				{ return jobinfos_.size()+failedjobs_.size(); }
    JobInfo*			currentJob(const HostNFailInfo*) const;

    int				nextStep()	{ return doCycle(); }
    int				nrDone() const	{ return jobsDone(); }
    int				totalNr() const	{ return totalJobs(); }
    const char*			message() const;
    const char*			nrDoneMessage() const;

    				// Set these before first step
    void			setFirstPort( int n )	    { firstport_ = n; }
    void			setRshComm( const char* s ) { rshcomm_ = s; }
    void			setProg( const char* s )    { prog_ = s; }
    				// Set this anytime
    void			setNiceNess( int n );

    void			showMachStatus( BufferStringSet& ) const;
    const FilePath&		getBaseFilePath(JobInfo&, const HostData&);

    Notifier<JobRunner>		preJobStart;
    Notifier<JobRunner>		postJobStart;
    Notifier<JobRunner>		jobFailed;
    Notifier<JobRunner>		msgAvail;

    const JobInfo&		curJobInfo() const	{ return *curjobinfo_; }
    IOPar&			curJobIOPar()		{ return curjobiop_; }
    const FilePath&		curJobFilePath()	{ return curjobfp_; }

    const char*			procDir() const	{ return procdir_.buf(); }
    				// processing directory on local machine

protected:

    JobDescProv*		descprov_;
    ObjectSet<JobInfo>		jobinfos_;
    ObjectSet<HostNFailInfo>	hostinfo_;
    ObjectSet<JobInfo>		failedjobs_;
    BufferString		prog_;
    BufferString		procdir_;
    FilePath&			curjobfp_;
    IOPar&			curjobiop_;
    JobInfo*			curjobinfo_;

    JobIOMgr&			iomgr();
    JobIOMgr*			iomgr__;

    int				niceval_;
    int				firstport_;
    BufferString		rshcomm_;
    int				maxhostfailures_;
    int				maxjobfailures_;
    int				jobtimeout_; 
    int				hosttimeout_;

    int				doCycle();
    HostNFailInfo*		hostNFailInfoFor(const HostData*) const;

    void			updateJobInfo();
    void 			handleStatusInfo( StatusInfo& );
    JobInfo* 			gtJob( int descnr );

    void 			failedJob( JobInfo&, JobInfo::State );

    enum StartRes		{ Started, NotStarted, JobBad, HostBad };
    StartRes			startJob( JobInfo& ji, HostNFailInfo& jhi );
    bool			runJob(JobInfo&,const HostData&);
    bool			assignJob(HostNFailInfo&);
    bool			haveIncomplete() const;

    enum HostStat		{ OK = 0, SomeFailed = 1, HostFailed = 2 };
    HostStat			hostStatus(const HostNFailInfo*) const;

};

#endif
