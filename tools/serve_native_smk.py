"""Felucca SMK Windows USB helper and local preview."""
import argparse,json,secrets
from native_smk import Bridge
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlparse,parse_qs
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--port',type=int,default=8893)
    p.add_argument('--allow-hosted',action='store_true',help='Allow the official Felucca site to use this loopback USB helper')
    hosted_origin='https://chancethemaker.github.io'
    api_token=secrets.token_urlsafe(32)
    a=p.parse_args();bridge=Bridge()
    class Handler(SimpleHTTPRequestHandler):
        def __init__(self,*args,**kwargs):super().__init__(*args,directory=str(ROOT/'web'),**kwargs)
        def api_allowed(self,write=False):
            hosts=(f'127.0.0.1:{a.port}',f'localhost:{a.port}')
            origins=tuple('http://'+host for host in hosts)
            origin=self.headers.get('Origin')
            if self.headers.get('Host') not in hosts:return False
            if origin in (origins if write else (None,*origins)):return True
            return a.allow_hosted and origin==hosted_origin and (not write or secrets.compare_digest(self.headers.get('X-Felucca-Token',''),api_token))
        def json_reply(self,value,status=200):
            data=json.dumps(value).encode();self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
        def do_OPTIONS(self):
            if not urlparse(self.path).path.startswith('/api/smk/') or not self.api_allowed():self.send_error(403);return
            if self.headers.get('Access-Control-Request-Method') not in ('GET','POST'):self.send_error(403);return
            requested={x.strip().lower() for x in self.headers.get('Access-Control-Request-Headers','').split(',') if x.strip()}
            if requested-{'content-type','x-felucca-token'}:self.send_error(403);return
            self.send_response(204)
            self.send_header('Access-Control-Allow-Methods','GET, POST')
            self.send_header('Access-Control-Allow-Headers','Content-Type, X-Felucca-Token')
            self.send_header('Access-Control-Allow-Private-Network','true')
            self.end_headers()
        def do_POST(self):
            if urlparse(self.path).path!='/api/smk/jobs':self.send_error(404);return
            if not self.api_allowed(write=True):self.send_error(403);return
            try:
                size=int(self.headers.get('Content-Length','0'))
                if not 0<size<=512 or self.headers.get('Content-Type','').split(';')[0]!='application/json':raise ValueError('Invalid request')
                request=json.loads(self.rfile.read(size))
                if set(request)!={'action','model','app'} or request['model']!='smk-pro' or request['app']!='felucca-smk':raise ValueError('Unsupported local device')
                self.json_reply({'id':bridge.start(request['action'])},202)
            except (ValueError,KeyError,TypeError) as exc:self.json_reply({'error':str(exc)},400)
        def do_GET(self):
            u=urlparse(self.path)
            if u.path.startswith('/api/smk/'):
                if not self.api_allowed():self.send_error(403);return
                if u.path=='/api/smk/capabilities':self.json_reply({'available':bridge.available,'models':['smk-pro'] if bridge.available else [],'protocol':1,'token':api_token,'app':'felucca-smk','firmware':'SMK-37_928'});return
                if u.path.startswith('/api/smk/jobs/'):
                    job=bridge.status(u.path.removeprefix('/api/smk/jobs/'))
                    self.json_reply(job if job else {'error':'Unknown operation'},200 if job else 404);return
                self.send_error(404);return
            super().do_GET()
        def local_allowed(self):
            hosts=(f'127.0.0.1:{a.port}',f'localhost:{a.port}')
            return self.headers.get('Host') in hosts and self.headers.get('Origin') in (None,*(('http://'+host) for host in hosts))
        def end_headers(self):
            self.send_header('Cache-Control','no-store')
            if a.allow_hosted and self.headers.get('Origin')==hosted_origin and urlparse(self.path).path.startswith('/api/smk/'):
                self.send_header('Access-Control-Allow-Origin',hosted_origin)
                self.send_header('Vary','Origin')
            super().end_headers()
    print(f'Felucca SMK: http://localhost:{a.port}/',flush=True)
    ThreadingHTTPServer(('127.0.0.1',a.port),Handler).serve_forever()
if __name__=='__main__':main()
