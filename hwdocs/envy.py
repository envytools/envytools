from sphinx.domains import Domain

class EnvyDomain(Domain):
    name = 'envy'
    label = 'envytools'
    object_types = {
    }
    directives = {
    }
    roles = {
    }
    initial_data = {
    }
    data_version = 0

def setup(app):
    app.add_domain(EnvyDomain)
