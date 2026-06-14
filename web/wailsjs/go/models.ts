export namespace transport {
	
	export class CompileResult {
	    pdf: number[];
	    log: string;
	
	    static createFrom(source: any = {}) {
	        return new CompileResult(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.pdf = source["pdf"];
	        this.log = source["log"];
	    }
	}
	export class FileDTO {
	    id: number;
	    project_id: number;
	    parent_id?: number;
	    name: string;
	    is_dir: boolean;
	    content?: string;
	    created_at: string;
	    updated_at: string;
	    children?: FileDTO[];
	
	    static createFrom(source: any = {}) {
	        return new FileDTO(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.id = source["id"];
	        this.project_id = source["project_id"];
	        this.parent_id = source["parent_id"];
	        this.name = source["name"];
	        this.is_dir = source["is_dir"];
	        this.content = source["content"];
	        this.created_at = source["created_at"];
	        this.updated_at = source["updated_at"];
	        this.children = this.convertValues(source["children"], FileDTO);
	    }
	
		convertValues(a: any, classs: any, asMap: boolean = false): any {
		    if (!a) {
		        return a;
		    }
		    if (a.slice && a.map) {
		        return (a as any[]).map(elem => this.convertValues(elem, classs));
		    } else if ("object" === typeof a) {
		        if (asMap) {
		            for (const key of Object.keys(a)) {
		                a[key] = new classs(a[key]);
		            }
		            return a;
		        }
		        return new classs(a);
		    }
		    return a;
		}
	}
	export class ProjectDTO {
	    id: number;
	    name: string;
	    created_at: string;
	    updated_at: string;
	
	    static createFrom(source: any = {}) {
	        return new ProjectDTO(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.id = source["id"];
	        this.name = source["name"];
	        this.created_at = source["created_at"];
	        this.updated_at = source["updated_at"];
	    }
	}

}

